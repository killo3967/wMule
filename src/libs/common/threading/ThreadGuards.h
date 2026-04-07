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

#ifndef THREADGUARDS_H
#define THREADGUARDS_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>

#include <wx/thread.h>

class CPreferences;

namespace Threading {

enum class ThreadOwnerTag {
	DownloadQueue,
	Throttler,
	Scheduler
};

enum class DrainResult {
	Completed = 0,
	TimedOut = 1
};

struct ThreadOwnershipSnapshot {
	uint64_t wakeups = 0;
	uint32_t pendingTasks = 0;
	uint32_t throttlerDebt = 0;
	uint64_t schedulerWaitMs = 0;
	uint32_t timedOutPartFiles = 0;
	wxThreadIdType downloadQueueOwner = wxThreadIdType();
	wxThreadIdType throttlerOwner = wxThreadIdType();
	DrainResult lastDrainResult = DrainResult::Completed;
};

struct ThreadPrefsSnapshot {
	uint32_t downLimit = 0;
	uint32_t upLimit = 0;
	bool verboseThreading = false;
	std::chrono::milliseconds drainTimeout{0};
};

ThreadPrefsSnapshot CaptureThreadPrefs();
ThreadPrefsSnapshot CaptureThreadPrefs(const CPreferences& prefs);

struct StatsSnapshot {
	ThreadOwnershipSnapshot ownership;
	ThreadPrefsSnapshot prefs;
	uint64_t sessionUploadedBytes = 0;
	uint64_t sessionDownloadedBytes = 0;
	uint32_t activeUploads = 0;
	uint32_t waitingUploads = 0;
	uint32_t activeDownloads = 0;
};

StatsSnapshot CaptureStatsSnapshot(const CPreferences& prefs);

struct PartFileAsyncSnapshot {
	uint32_t activeTasks = 0;
	bool shuttingDown = false;
	std::chrono::milliseconds lastWait{0};
	bool lastTimedOut = false;
};

class PartFileAsyncGate {
public:
	bool BeginTask();
	void EndTask();
	void RequestShutdown(std::chrono::milliseconds timeout);
	bool IsShuttingDown() const;
	PartFileAsyncSnapshot Snapshot() const;

private:
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	uint32_t m_activeTasks = 0;
	bool m_blocked = false;
	std::chrono::milliseconds m_lastWait{0};
	bool m_lastTimedOut = false;
};

class ThreadMetrics {
public:
	static ThreadMetrics& Instance();

	void IncrementPendingTasks();
	void DecrementPendingTasks();
	void AddSchedulerWait(std::chrono::milliseconds waitMs);
	void RegisterThrottlerDebt(uint32_t debt);
	void RegisterWakeup();
	void RegisterPartFileTimeout();
	void RegisterDrainResult(DrainResult result);
	void ResetThrottlerDebt();
	void SetDownloadQueueOwner(wxThreadIdType id);
	void ClearDownloadQueueOwner();
	void SetThrottlerOwner(wxThreadIdType id);
	void ClearThrottlerOwner();
	ThreadOwnershipSnapshot Snapshot() const;
	void ResetForTests();

private:
	ThreadMetrics();

	std::atomic<uint32_t> m_pendingTasks;
	std::atomic<uint32_t> m_throttlerDebt;
	std::atomic<uint64_t> m_wakeups;
	std::atomic<uint64_t> m_schedulerWaitMs;
	std::atomic<uint32_t> m_timedOutPartFiles;
	std::atomic<int> m_lastDrainResult;
	mutable std::mutex m_ownerMutex;
	wxThreadIdType m_downloadQueueOwner;
	wxThreadIdType m_throttlerOwner;
};

class ScopedThreadOwner {
public:
	explicit ScopedThreadOwner(ThreadOwnerTag tag);
	~ScopedThreadOwner();

private:
	ThreadOwnerTag m_tag;
};

class ScopedQueueLock {
public:
	ScopedQueueLock(std::mutex& mutex, ThreadOwnerTag tag);
	~ScopedQueueLock();

private:
	std::mutex& m_mutex;
	ThreadOwnerTag m_tag;
	bool m_locked;
	std::unique_ptr<ScopedThreadOwner> m_owner;
};

class ThreadShutdownToken {
public:
	ThreadShutdownToken();

	void Activate();
	bool IsActive() const;
	bool GuardEnqueueOrErr(const char* origin) const;

private:
	mutable std::atomic<bool> m_active;
};

ThreadShutdownToken& ShutdownToken();

} // namespace Threading

#endif // THREADGUARDS_H
