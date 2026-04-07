#include <muleunit/test.h>

#include <common/threading/ThreadGuards.h>

#include <chrono>
#include <thread>

using namespace muleunit;

DECLARE_SIMPLE(PartFileAsyncTaskTest);

TEST(PartFileAsyncTaskTest, RequestShutdownWaitsForTasks)
{
	Threading::ThreadMetrics::Instance().ResetForTests();
	Threading::PartFileAsyncGate gate;
	ASSERT_TRUE(gate.BeginTask());
	std::thread worker([&gate]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		gate.EndTask();
	});
	gate.RequestShutdown(std::chrono::milliseconds(200));
	worker.join();
	const Threading::PartFileAsyncSnapshot snapshot = gate.Snapshot();
	ASSERT_FALSE(snapshot.lastTimedOut);
	ASSERT_EQUALS(0u, snapshot.activeTasks);
	ASSERT_TRUE(snapshot.lastWait.count() >= 20);
}

TEST(PartFileAsyncTaskTest, RequestShutdownTimesOut)
{
	Threading::ThreadMetrics::Instance().ResetForTests();
	Threading::PartFileAsyncGate gate;
	ASSERT_TRUE(gate.BeginTask());
	std::thread worker([&gate]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		gate.EndTask();
	});
	gate.RequestShutdown(std::chrono::milliseconds(10));
	const Threading::PartFileAsyncSnapshot snapshot = gate.Snapshot();
	ASSERT_TRUE(snapshot.lastTimedOut);
	worker.join();
	ASSERT_TRUE(Threading::ThreadMetrics::Instance().Snapshot().timedOutPartFiles >= 1);
}
