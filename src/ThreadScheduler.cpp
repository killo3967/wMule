//
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2011 Mikkel Schubert ( xaignar@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "ThreadScheduler.h"	// Interface declarations
#include "Logger.h"				// Needed for Add(Debug)LogLine{C,N}
#include <common/Format.h>		// Needed for CFormat
#include <common/ThreadPool.h>	// Needed for parallel task execution
#include <common/threading/ThreadGuards.h>
#include "ScopedPtr.h"			// Needed for CScopedPtr

#include <algorithm>			// Needed for std::sort		// Do_not_auto_remove (mingw-gcc-3.4.5)
#include <chrono>			// Needed for std::chrono::milliseconds
#include <wx/thread.h>

//! Global lock the scheduler and its thread.
static wxMutex s_lock;
static wxCondition s_taskEvent(s_lock);
//! Pointer to the global scheduler instance (automatically instantiated).
static CThreadScheduler* s_scheduler = nullptr;
//! Specifies if the scheduler is running.
static bool	s_running = false;
//! Specifies if the global scheduler has been terminated.
static bool s_terminated = false;

/**
 * This class is used in a custom implementation of wxThreadHelper.
 *
 * The reason for not using wxThreadHelper are as follows:
 *  - wxThreadHelper makes use of wxThread:Kill, which is warned against
 *    several times in the docs, and even calls it in its destructor.
 *  - Managing the thread-object is difficult, since the only way to
 *    destroy it is to create a new thread.
 */
class CTaskThread : public CMuleThread
{
public:
	CTaskThread(CThreadScheduler* owner)
		: CMuleThread(wxTHREAD_JOINABLE),
		  m_owner(owner)
	{
	}

	//! For simplicity's sake, all code is placed in CThreadScheduler::Entry
	void* Entry() {
		return m_owner->Entry();
	}

private:
	//! The scheduler owning this thread.
	CThreadScheduler* m_owner;
};


void CThreadScheduler::Start()
{
	wxMutexLocker lock(s_lock);

	s_running = true;
	s_terminated = false;

	// Ensures that a thread is started if tasks are already waiting.
	if (s_scheduler) {
		AddDebugLogLineN(logThreads, wxT("Starting scheduler"));
		s_scheduler->CreateSchedulerThread();
	}
}


void CThreadScheduler::Terminate()
{
	AddDebugLogLineN(logThreads, wxT("Terminating scheduler"));
	CThreadScheduler* ptr = nullptr;

	{
		wxMutexLocker lock(s_lock);

		// Safely unlink the scheduler, as to avoid race-conditions.
		ptr = s_scheduler;
		s_running = false;
		s_terminated = true;
		s_scheduler = nullptr;
	}

	delete ptr;
	AddDebugLogLineN(logThreads, wxT("Scheduler terminated"));
}


bool CThreadScheduler::AddTask(CThreadTask* task, bool overwrite)
{
	if (!Threading::ShutdownToken().GuardEnqueueOrErr("CThreadScheduler::AddTask")) {
		delete task;
		return false;
	}

	wxMutexLocker lock(s_lock);

	// When terminated (on shutdown), all tasks are ignored.
	if (s_terminated) {
		AddDebugLogLineN(logThreads, wxT("Task discarded: ") + task->GetDesc());
		delete task;
		return false;
	} else if (s_scheduler == nullptr) {
		s_scheduler = new CThreadScheduler();
		AddDebugLogLineN(logThreads, wxT("Scheduler created."));
	}

	return s_scheduler->DoAddTask(task, overwrite);
}


/** Returns string representation of error code. */
static wxString GetErrMsg(wxThreadError err)
{
	switch (err) {
		case wxTHREAD_NO_ERROR:		return wxT("wxTHREAD_NO_ERROR");
		case wxTHREAD_NO_RESOURCE:	return wxT("wxTHREAD_NO_RESOURCE");
		case wxTHREAD_RUNNING:		return wxT("wxTHREAD_RUNNING");
		case wxTHREAD_NOT_RUNNING:	return wxT("wxTHREAD_NOT_RUNNING");
		case wxTHREAD_KILLED:		return wxT("wxTHREAD_KILLED");
		case wxTHREAD_MISC_ERROR:	return wxT("wxTHREAD_MISC_ERROR");
		default:
			return wxT("Unknown error");
	}
}


void CThreadScheduler::CreateSchedulerThread()
{
	if ((m_thread && m_thread->IsAlive()) || m_tasks.empty()) {
		return;
	}

	// A thread can only be run once, so the old one must be safely disposed of
	if (m_thread) {
		AddDebugLogLineN(logThreads, wxT("CreateSchedulerThread: Disposing of old thread."));
		m_thread->Stop();
		delete m_thread;
	}

	m_thread = new CTaskThread(this);

	wxThreadError err = m_thread->Create();
	if (err == wxTHREAD_NO_ERROR) {
		// Try to avoid reducing the latency of the main thread
		m_thread->SetPriority(WXTHREAD_MIN_PRIORITY);

		err = m_thread->Run();
		if (err == wxTHREAD_NO_ERROR) {
			AddDebugLogLineN(logThreads, wxT("Scheduler thread started"));
			return;
		} else {
			AddDebugLogLineC(logThreads, wxT("Error while starting scheduler thread: ") + GetErrMsg(err));
		}
	} else {
		AddDebugLogLineC(logThreads, wxT("Error while creating scheduler thread: ") + GetErrMsg(err));
	}

	// Creation or running failed.
	m_thread->Stop();
	delete m_thread;
	m_thread = nullptr;
}


/** This is the sorter functor for the task-queue. */
struct CTaskSorter
{
	bool operator()(const CThreadScheduler::CEntryPair& a, const CThreadScheduler::CEntryPair& b) {
		if (a.first->GetPriority() != b.first->GetPriority()) {
			return a.first->GetPriority() > b.first->GetPriority();
		}

		// Compare tasks numbers.
		return a.second < b.second;
	}
};



CThreadScheduler::CThreadScheduler()
	: m_tasksDirty(false),
	  m_thread(nullptr),
	  m_currentTask(nullptr),
	  m_activeTasks(0)
{

}


CThreadScheduler::~CThreadScheduler()
{
	if (m_thread) {
		m_thread->Stop();
		delete m_thread;
	}
}


size_t CThreadScheduler::GetTaskCount() const
{
	wxMutexLocker lock(s_lock);

	return m_tasks.size();
}


bool CThreadScheduler::DoAddTask(CThreadTask* task, bool overwrite)
{
	// GetTick is too lowres, so we just use a counter to ensure that
	// the sorted order will match the order in which the tasks were added.
	static unsigned taskAge = 0;

	// Get the map for this task type, implicitly creating it as needed.
	CDescMap& map = m_taskDescs[task->GetType()];

	CDescMap::value_type entry(task->GetDesc(), task);
	if (map.insert(entry).second) {
		AddDebugLogLineN(logThreads, wxT("Task scheduled: ") + task->GetType() + wxT(" - ") + task->GetDesc());
		m_tasks.push_back(CEntryPair(task, taskAge++));
		m_tasksDirty = true;
	} else if (overwrite) {
		AddDebugLogLineN(logThreads, wxT("Task overwritten: ") + task->GetType() + wxT(" - ") + task->GetDesc());

		CThreadTask* existingTask = map[task->GetDesc()];
		if (m_currentTask == existingTask) {
			// The duplicate is already being executed, abort it.
			m_currentTask->m_abort = true;
		} else {
			// Task not yet started, simply remove and delete.
			wxCHECK2(map.erase(existingTask->GetDesc()), /* Do nothing. */);
			delete existingTask;
		}

		m_tasks.push_back(CEntryPair(task, taskAge++));
		map[task->GetDesc()] = task;
		m_tasksDirty = true;
	} else {
		AddDebugLogLineN(logThreads, wxT("Duplicate task, discarding: ") + task->GetType() + wxT(" - ") + task->GetDesc());
		delete task;
		return false;
	}

	if (s_running) {
		CreateSchedulerThread();
	}

	s_taskEvent.Signal();

	return true;
}


void* CThreadScheduler::Entry()
{
	AddDebugLogLineN(logThreads, wxT("Entering scheduling loop"));

	CThreadPool& pool = CThreadPool::Instance();

 	while (true) {
		Threading::ScopedThreadOwner owner(Threading::ThreadOwnerTag::Scheduler);
		CThreadTask* rawTask = nullptr;

		{
			wxMutexLocker lock(s_lock);

			while (m_tasks.empty() && m_activeTasks > 0 && !m_thread->TestDestroy()) {
				s_taskEvent.Wait();
			}

			if (m_thread->TestDestroy()) {
				break;
			}

			if (m_tasks.empty() && m_activeTasks == 0) {
				AddDebugLogLineN(logThreads, wxT("No more tasks, stopping"));
				break;
			}

			if (!m_tasks.empty()) {
				if (m_tasksDirty) {
					AddDebugLogLineN(logThreads, wxT("Resorting tasks"));
					std::sort(m_tasks.begin(), m_tasks.end(), CTaskSorter());
					m_tasksDirty = false;
				}

				rawTask = m_tasks.front().first;
				m_tasks.pop_front();
				m_currentTask = rawTask;
				++m_activeTasks;
			}
		}

		if (!rawTask) {
			continue;
		}

		AddDebugLogLineN(logThreads, wxT("Current task: ") + rawTask->GetType() + wxT(" - ") + rawTask->GetDesc());
		rawTask->m_owner = m_thread;

		pool.Enqueue([this, rawTask]() {
			rawTask->Entry();
			rawTask->OnExit();

			// Check if this was the last task of this type
			bool isLastTask = false;

			{
				wxMutexLocker lock(s_lock);

				if (!rawTask->m_abort) {
					AddDebugLogLineN(logThreads,
						CFormat(wxT("Completed task '%s%s', %u tasks remaining."))
							% rawTask->GetType()
							% (rawTask->GetDesc().IsEmpty() ? wxString() : (wxT(" - ") + rawTask->GetDesc()))
							% m_tasks.size());

					CDescMap& map = m_taskDescs[rawTask->GetType()];
					if (!map.erase(rawTask->GetDesc())) {
						wxFAIL;
					} else if (map.empty()) {
						m_taskDescs.erase(rawTask->GetType());
						isLastTask = true;
					}
				}

				m_currentTask = nullptr;
				--m_activeTasks;
				s_taskEvent.Signal();
			}

			if (isLastTask) {
				AddDebugLogLineN(logThreads, wxT("Last task, calling OnLastTask"));
				rawTask->OnLastTask();
			}

			delete rawTask;
		}, true);
	}

	// Wait for all active tasks to complete before exiting
	pool.WaitAll();

	AddDebugLogLineN(logThreads, wxT("Leaving scheduling loop"));

	return 0;
}



CThreadTask::CThreadTask(const wxString& type, const wxString& desc, ETaskPriority priority)
	: m_type(type),
	  m_desc(desc),
	  m_priority(priority),
	  m_owner(nullptr),
	  m_abort(false)
{
}


CThreadTask::~CThreadTask()
{
}


void CThreadTask::OnLastTask()
{
	// Does nothing by default.
}


void CThreadTask::OnExit()
{
	// Does nothing by default.
}


bool CThreadTask::TestDestroy() const
{
	wxCHECK(m_owner, m_abort);

	return m_abort || m_owner->TestDestroy();
}


const wxString& CThreadTask::GetType() const
{
	return m_type;
}


const wxString& CThreadTask::GetDesc() const
{
	return m_desc;
}


ETaskPriority CThreadTask::GetPriority() const
{
	return m_priority;
}


// File_checked_for_headers
Threading::DrainResult CThreadScheduler::Drain(std::chrono::milliseconds timeout)
{
	wxMutexLocker lock(s_lock);
	if (!s_scheduler) {
		return Threading::DrainResult::Completed;
	}

	const auto start = std::chrono::steady_clock::now();
	Threading::DrainResult result = Threading::DrainResult::Completed;
	const auto hasWork = []() {
		return (s_scheduler != nullptr) && (!s_scheduler->m_tasks.empty() || s_scheduler->m_activeTasks > 0);
	};

	while (hasWork()) {
		if (timeout.count() <= 0) {
			s_taskEvent.Wait();
		} else {
			const auto now = std::chrono::steady_clock::now();
			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
			if (elapsed >= timeout) {
				result = Threading::DrainResult::TimedOut;
				break;
			}
			const auto remaining = timeout - elapsed;
			const wxCondError waitResult = s_taskEvent.WaitTimeout(static_cast<long>(std::max<std::chrono::milliseconds>(remaining, std::chrono::milliseconds(10)).count()));
			if (waitResult == wxCOND_TIMEOUT && hasWork()) {
				result = Threading::DrainResult::TimedOut;
				break;
			}
		}
	}

	Threading::ThreadMetrics::Instance().RegisterDrainResult(result);
	if (result == Threading::DrainResult::TimedOut) {
		AddDebugLogLineC(logThreads, wxT("ThreadScheduler drain timed out"));
	}
	return result;
}
