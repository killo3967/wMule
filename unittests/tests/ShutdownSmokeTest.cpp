#include <muleunit/test.h>

#include <common/threading/ThreadGuards.h>

#include <mutex>

using namespace muleunit;

DECLARE_SIMPLE(ShutdownSmokeTest);

TEST(ShutdownSmokeTest, OwnershipSnapshotTracksPendingTasks)
{
	Threading::ThreadMetrics::Instance().ResetForTests();
	std::mutex queueMutex;
	{
		Threading::ScopedQueueLock guard(queueMutex, Threading::ThreadOwnerTag::DownloadQueue);
		const Threading::ThreadOwnershipSnapshot snapshot = Threading::ThreadMetrics::Instance().Snapshot();
		ASSERT_EQUALS(1u, snapshot.pendingTasks);
	}
	ASSERT_EQUALS(0u, Threading::ThreadMetrics::Instance().Snapshot().pendingTasks);
}

TEST(ShutdownSmokeTest, DrainResultIsPublished)
{
	Threading::ThreadMetrics::Instance().ResetForTests();
	Threading::ThreadMetrics::Instance().RegisterDrainResult(Threading::DrainResult::TimedOut);
	ASSERT_EQUALS(static_cast<int>(Threading::DrainResult::TimedOut),
		static_cast<int>(Threading::ThreadMetrics::Instance().Snapshot().lastDrainResult));
}
