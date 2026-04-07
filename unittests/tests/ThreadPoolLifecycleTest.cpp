#include <muleunit/test.h>

#include <libs/common/ThreadPool.h>
#include <common/threading/ThreadGuards.h>

#include <chrono>
#include <thread>

using namespace muleunit;

DECLARE_SIMPLE(ThreadPoolLifecycleTest);

TEST(ThreadPoolLifecycleTest, DrainCompletesWithinTimeout)
{
	Threading::ThreadMetrics::Instance().ResetForTests();
	CThreadPool& pool = CThreadPool::Instance();
	pool.Enqueue([]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	});
	const Threading::DrainResult result = pool.Drain(std::chrono::milliseconds(200));
	ASSERT_EQUALS(static_cast<int>(Threading::DrainResult::Completed), static_cast<int>(result));
}

TEST(ThreadPoolLifecycleTest, DrainTimesOutWhenWorkStalls)
{
	Threading::ThreadMetrics::Instance().ResetForTests();
	CThreadPool& pool = CThreadPool::Instance();
	pool.Enqueue([]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
	});
	ASSERT_EQUALS(static_cast<int>(Threading::DrainResult::TimedOut),
		static_cast<int>(pool.Drain(std::chrono::milliseconds(10))));
	ASSERT_EQUALS(static_cast<int>(Threading::DrainResult::Completed),
		static_cast<int>(pool.Drain(std::chrono::milliseconds(500))));
}
