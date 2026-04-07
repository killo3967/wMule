#include <muleunit/test.h>

#include <common/threading/ThreadGuards.h>
#include <Preferences.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace muleunit;
using namespace Threading;

DECLARE_SIMPLE(ThreadingGuardsTest);

TEST(ThreadingGuardsTest, ScopedQueueLockUpdatesPendingTasks)
{
	ThreadMetrics::Instance().ResetForTests();

	std::mutex mutex;
	{
		ScopedQueueLock guard(mutex, ThreadOwnerTag::DownloadQueue);
		const ThreadOwnershipSnapshot snapshot = ThreadMetrics::Instance().Snapshot();
		ASSERT_EQUALS(1u, snapshot.pendingTasks);
		ASSERT_EQUALS(wxThread::GetCurrentId(), snapshot.downloadQueueOwner);
	}

	ASSERT_EQUALS(0u, ThreadMetrics::Instance().Snapshot().pendingTasks);
}

TEST(ThreadingGuardsTest, ScopedQueueLockCapturesWaitDuration)
{
	ThreadMetrics::Instance().ResetForTests();

	std::mutex mutex;
	std::mutex readinessMutex;
	std::condition_variable readiness;
	bool holderReady = false;

	std::thread holder([&]() {
		std::unique_lock<std::mutex> lock(mutex);
		{
			std::lock_guard<std::mutex> readinessLock(readinessMutex);
			holderReady = true;
		}
		readiness.notify_one();
		std::this_thread::sleep_for(std::chrono::milliseconds(15));
	});

	{
		std::unique_lock<std::mutex> readinessLock(readinessMutex);
		readiness.wait(readinessLock, [&holderReady]() {
			return holderReady;
		});
	}

	ScopedQueueLock guard(mutex, ThreadOwnerTag::DownloadQueue);
	holder.join();

	const ThreadOwnershipSnapshot snapshot = ThreadMetrics::Instance().Snapshot();
	ASSERT_TRUE(snapshot.schedulerWaitMs >= 10);
}

TEST(ThreadingGuardsTest, ShutdownTokenRejectsNewWorkAfterActivation)
{
	ThreadShutdownToken token;
	ASSERT_FALSE(token.IsActive());
	ASSERT_TRUE(token.GuardEnqueueOrErr("ThreadingGuardsTest"));

	token.Activate();
	ASSERT_TRUE(token.IsActive());
	ASSERT_FALSE(token.GuardEnqueueOrErr("ThreadingGuardsTest"));
}

TEST(ThreadingGuardsTest, ThreadPrefsSnapshotReflectsPreferences)
{
	const bool originalVerbose = thePrefs::GetVerboseThreading();
	const uint16 originalDown = thePrefs::GetMaxDownload();
	const uint16 originalUp = thePrefs::GetMaxUpload();
	const uint32 originalTimeout = thePrefs::GetThreadDrainTimeoutMs();

	thePrefs::SetVerboseThreading(true);
	thePrefs::SetMaxDownload(600);
	thePrefs::SetMaxUpload(120);
	thePrefs::SetThreadDrainTimeoutMs(9000);

	const ThreadPrefsSnapshot snapshot = CaptureThreadPrefs();
	ASSERT_EQUALS(600u, snapshot.downLimit);
	ASSERT_EQUALS(120u, snapshot.upLimit);
	ASSERT_TRUE(snapshot.verboseThreading);
	ASSERT_EQUALS(9000u, static_cast<uint32>(snapshot.drainTimeout.count()));

	thePrefs::SetMaxDownload(originalDown);
	thePrefs::SetMaxUpload(originalUp);
	thePrefs::SetVerboseThreading(originalVerbose);
	thePrefs::SetThreadDrainTimeoutMs(originalTimeout);
}
