#include <muleunit/test.h>

#include <common/threading/ThreadGuards.h>

#include <wx/thread.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace muleunit;

DECLARE_SIMPLE(DownloadQueueRaceTest);

TEST(DownloadQueueRaceTest, WritersSerializeAccess)
{
	Threading::ThreadMetrics::Instance().ResetForTests();
	std::mutex queueMutex;
	std::atomic<int> order{0};
	std::atomic<bool> secondEntered{false};
	std::atomic<int> observedOrder{0};
	std::mutex readinessMutex;
	std::condition_variable readiness;
	bool firstHoldingLock = false;

	std::thread first([&]() {
		Threading::ScopedQueueLock guard(queueMutex, Threading::ThreadOwnerTag::DownloadQueue);
		order.store(1);
		{
			std::lock_guard<std::mutex> readinessLock(readinessMutex);
			firstHoldingLock = true;
		}
		readiness.notify_one();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	});

	{
		std::unique_lock<std::mutex> readinessLock(readinessMutex);
		readiness.wait(readinessLock, [&firstHoldingLock]() {
			return firstHoldingLock;
		});
	}

	std::thread second([&]() {
		Threading::ScopedQueueLock guard(queueMutex, Threading::ThreadOwnerTag::DownloadQueue);
		secondEntered.store(true);
		observedOrder.store(order.load());
	});

	first.join();
	second.join();
	ASSERT_TRUE(secondEntered.load());
	ASSERT_EQUALS(1, observedOrder.load());
	ASSERT_EQUALS(0u, Threading::ThreadMetrics::Instance().Snapshot().pendingTasks);
}

TEST(DownloadQueueRaceTest, MetricsExposeDownloadQueueOwner)
{
	Threading::ThreadMetrics::Instance().ResetForTests();
	std::mutex queueMutex;
	Threading::ScopedQueueLock guard(queueMutex, Threading::ThreadOwnerTag::DownloadQueue);
	const Threading::ThreadOwnershipSnapshot snapshot = Threading::ThreadMetrics::Instance().Snapshot();
	ASSERT_EQUALS(wxThread::GetCurrentId(), snapshot.downloadQueueOwner);
}
