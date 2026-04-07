//
// This file is part of the aMule Project.
//
// Copyright (c) 2026 wMule Team
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

#include <common/threading/ThreadGuards.h>

#include "Logger.h"
#include "Preferences.h"
#include "Statistics.h"

#include <common/Format.h>

#include <wx/string.h>
#include <wx/thread.h>

namespace Threading {

namespace {

wxThreadIdType NullThreadId()
{
	return wxThreadIdType();
}

} // namespace

ThreadMetrics& ThreadMetrics::Instance()
{
	static ThreadMetrics instance;
	return instance;
}

ThreadMetrics::ThreadMetrics()
	: m_pendingTasks(0)
	, m_throttlerDebt(0)
	, m_wakeups(0)
	, m_schedulerWaitMs(0)
	, m_timedOutPartFiles(0)
	, m_lastDrainResult(static_cast<int>(DrainResult::Completed))
	, m_downloadQueueOwner(NullThreadId())
	, m_throttlerOwner(NullThreadId())
{
}

void ThreadMetrics::IncrementPendingTasks()
{
	m_pendingTasks.fetch_add(1, std::memory_order_relaxed);
}

void ThreadMetrics::DecrementPendingTasks()
{
	uint32_t expected = m_pendingTasks.load(std::memory_order_relaxed);
	while (expected > 0 && !m_pendingTasks.compare_exchange_weak(expected, expected - 1, std::memory_order_relaxed)) {
		// retry
	}
}

void ThreadMetrics::AddSchedulerWait(std::chrono::milliseconds waitMs)
{
	m_schedulerWaitMs.fetch_add(waitMs.count(), std::memory_order_relaxed);
}

void ThreadMetrics::RegisterThrottlerDebt(uint32_t debt)
{
	m_throttlerDebt.store(debt, std::memory_order_relaxed);
}

void ThreadMetrics::RegisterWakeup()
{
	m_wakeups.fetch_add(1, std::memory_order_relaxed);
}

void ThreadMetrics::RegisterPartFileTimeout()
{
	m_timedOutPartFiles.fetch_add(1, std::memory_order_relaxed);
}

void ThreadMetrics::RegisterDrainResult(DrainResult result)
{
	m_lastDrainResult.store(static_cast<int>(result), std::memory_order_relaxed);
}

void ThreadMetrics::ResetThrottlerDebt()
{
	m_throttlerDebt.store(0, std::memory_order_relaxed);
}

void ThreadMetrics::SetDownloadQueueOwner(wxThreadIdType id)
{
	std::lock_guard<std::mutex> lock(m_ownerMutex);
	m_downloadQueueOwner = id;
}

void ThreadMetrics::ClearDownloadQueueOwner()
{
	std::lock_guard<std::mutex> lock(m_ownerMutex);
	m_downloadQueueOwner = NullThreadId();
}

void ThreadMetrics::SetThrottlerOwner(wxThreadIdType id)
{
	std::lock_guard<std::mutex> lock(m_ownerMutex);
	m_throttlerOwner = id;
}

void ThreadMetrics::ClearThrottlerOwner()
{
	std::lock_guard<std::mutex> lock(m_ownerMutex);
	m_throttlerOwner = NullThreadId();
}

ThreadOwnershipSnapshot ThreadMetrics::Snapshot() const
{
	ThreadOwnershipSnapshot snapshot;
	snapshot.pendingTasks = m_pendingTasks.load(std::memory_order_relaxed);
	snapshot.throttlerDebt = m_throttlerDebt.load(std::memory_order_relaxed);
	snapshot.wakeups = m_wakeups.load(std::memory_order_relaxed);
	snapshot.schedulerWaitMs = m_schedulerWaitMs.load(std::memory_order_relaxed);
	snapshot.timedOutPartFiles = m_timedOutPartFiles.load(std::memory_order_relaxed);
	snapshot.lastDrainResult = static_cast<DrainResult>(m_lastDrainResult.load(std::memory_order_relaxed));

	{
		std::lock_guard<std::mutex> lock(m_ownerMutex);
		snapshot.downloadQueueOwner = m_downloadQueueOwner;
		snapshot.throttlerOwner = m_throttlerOwner;
	}

	return snapshot;
}

void ThreadMetrics::ResetForTests()
{
	m_pendingTasks.store(0, std::memory_order_relaxed);
	m_throttlerDebt.store(0, std::memory_order_relaxed);
	m_wakeups.store(0, std::memory_order_relaxed);
	m_schedulerWaitMs.store(0, std::memory_order_relaxed);
	m_timedOutPartFiles.store(0, std::memory_order_relaxed);
	m_lastDrainResult.store(static_cast<int>(DrainResult::Completed), std::memory_order_relaxed);
	std::lock_guard<std::mutex> lock(m_ownerMutex);
	m_downloadQueueOwner = NullThreadId();
	m_throttlerOwner = NullThreadId();
}

ScopedThreadOwner::ScopedThreadOwner(ThreadOwnerTag tag)
	: m_tag(tag)
{
	wxThreadIdType id = wxThread::GetCurrentId();
	ThreadMetrics& metrics = ThreadMetrics::Instance();
	switch (m_tag) {
		case ThreadOwnerTag::DownloadQueue:
			metrics.SetDownloadQueueOwner(id);
			break;
		case ThreadOwnerTag::Throttler:
			metrics.SetThrottlerOwner(id);
			break;
		case ThreadOwnerTag::Scheduler:
			metrics.RegisterWakeup();
			break;
	}
}

ScopedThreadOwner::~ScopedThreadOwner()
{
	ThreadMetrics& metrics = ThreadMetrics::Instance();
	switch (m_tag) {
		case ThreadOwnerTag::DownloadQueue:
			metrics.ClearDownloadQueueOwner();
			break;
		case ThreadOwnerTag::Throttler:
			metrics.ClearThrottlerOwner();
			break;
		case ThreadOwnerTag::Scheduler:
			// Scheduler ownership is transient; no-op
			break;
	}
}

ScopedQueueLock::ScopedQueueLock(std::mutex& mutex, ThreadOwnerTag tag)
	: m_mutex(mutex)
	, m_tag(tag)
	, m_locked(false)
{
	const auto start = std::chrono::steady_clock::now();
	m_mutex.lock();
	m_locked = true;
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	ThreadMetrics::Instance().AddSchedulerWait(elapsed);
	ThreadMetrics::Instance().IncrementPendingTasks();
	m_owner = std::make_unique<ScopedThreadOwner>(m_tag);
}

ScopedQueueLock::~ScopedQueueLock()
{
	if (m_locked) {
		ThreadMetrics::Instance().DecrementPendingTasks();
		m_owner.reset();
		m_mutex.unlock();
	}
}

ThreadShutdownToken::ThreadShutdownToken()
	: m_active(false)
{
}

void ThreadShutdownToken::Activate()
{
	m_active.store(true, std::memory_order_release);
}

bool ThreadShutdownToken::IsActive() const
{
	return m_active.load(std::memory_order_acquire);
}

bool ThreadShutdownToken::GuardEnqueueOrErr(const char* origin) const
{
	if (!IsActive()) {
		return true;
	}

	const wxString originStr = wxString::FromUTF8(origin ? origin : "?");
	AddDebugLogLineC(logThreading, CFormat(wxT("Rejected request from %s: shutting down")) % originStr);
	return false;
}

ThreadShutdownToken& ShutdownToken()
{
	static ThreadShutdownToken token;
	return token;
}

ThreadPrefsSnapshot CaptureThreadPrefs()
{
	ThreadPrefsSnapshot snapshot;
	snapshot.downLimit = thePrefs::GetMaxDownload();
	snapshot.upLimit = thePrefs::GetMaxUpload();
	snapshot.verboseThreading = thePrefs::GetVerboseThreading();
	snapshot.drainTimeout = std::chrono::milliseconds(thePrefs::GetThreadDrainTimeoutMs());
	return snapshot;
}

ThreadPrefsSnapshot CaptureThreadPrefs(const CPreferences& WXUNUSED(prefs))
{
	return CaptureThreadPrefs();
}

StatsSnapshot CaptureStatsSnapshot(const CPreferences& prefs)
{
	StatsSnapshot snapshot;
	snapshot.ownership = ThreadMetrics::Instance().Snapshot();
	snapshot.prefs = CaptureThreadPrefs(prefs);
	const ThreadingStatsSnapshot stats = theStats::GetThreadingStatsSnapshot();
	snapshot.sessionUploadedBytes = stats.sessionUploadedBytes;
	snapshot.sessionDownloadedBytes = stats.sessionDownloadedBytes;
	snapshot.activeUploads = stats.activeUploads;
	snapshot.waitingUploads = stats.waitingUploads;
	snapshot.activeDownloads = stats.activeDownloads;
	return snapshot;
}

bool PartFileAsyncGate::BeginTask()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_blocked) {
		return false;
	}
	++m_activeTasks;
	return true;
}

void PartFileAsyncGate::EndTask()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_activeTasks == 0) {
		return;
	}
	--m_activeTasks;
	if (m_blocked && m_activeTasks == 0) {
		m_condition.notify_all();
	}
}

void PartFileAsyncGate::RequestShutdown(std::chrono::milliseconds timeout)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_blocked && m_activeTasks == 0) {
		m_lastWait = std::chrono::milliseconds(0);
		m_lastTimedOut = false;
		return;
	}

	m_blocked = true;
	const auto start = std::chrono::steady_clock::now();
	bool timedOut = false;

	while (m_activeTasks != 0) {
		if (timeout.count() <= 0) {
			m_condition.wait(lock);
		} else {
			const auto now = std::chrono::steady_clock::now();
			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
			if (elapsed >= timeout) {
				timedOut = true;
				break;
			}
			const auto remaining = timeout - elapsed;
			if (m_condition.wait_for(lock, remaining, [this]() { return m_activeTasks == 0; })) {
				break;
			}
		}
	}

	m_lastWait = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	m_lastTimedOut = timedOut && (m_activeTasks != 0);
	if (m_lastTimedOut) {
		ThreadMetrics::Instance().RegisterPartFileTimeout();
		AddDebugLogLineC(logThreading,
			CFormat(wxT("PartFileAsyncGate timed out after %lums with %u tasks"))
				% static_cast<unsigned long>(m_lastWait.count())
				% m_activeTasks);
	}
}

bool PartFileAsyncGate::IsShuttingDown() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_blocked;
}

PartFileAsyncSnapshot PartFileAsyncGate::Snapshot() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	PartFileAsyncSnapshot snapshot;
	snapshot.activeTasks = m_activeTasks;
	snapshot.shuttingDown = m_blocked;
	snapshot.lastWait = m_lastWait;
	snapshot.lastTimedOut = m_lastTimedOut;
	return snapshot;
}

} // namespace Threading
