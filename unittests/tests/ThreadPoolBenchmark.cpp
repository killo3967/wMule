//
// This file is part of the aMule Project.
//
// Copyright (c) 2024 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <muleunit/test.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

using namespace muleunit;

namespace {

double GetTimeMs() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double, std::chrono::milliseconds::period>(duration).count();
}

void SleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}

DECLARE_SIMPLE(ThreadPoolBenchmark);

TEST(ThreadPoolBenchmark, ThreadCreationOverhead)
{
    printf("\n=== Thread Creation Overhead ===\n");

    constexpr int ITERATIONS = 50;

    double totalTime = 0.0;
    double minTime = 999999.0;
    double maxTime = 0.0;

    for (int i = 0; i < ITERATIONS; ++i) {
        double start = GetTimeMs();
        std::thread t([]() {});
        t.join();
        double end = GetTimeMs();
        double elapsed = end - start;

        totalTime += elapsed;
        if (elapsed < minTime) minTime = elapsed;
        if (elapsed > maxTime) maxTime = elapsed;
    }

    double avg = totalTime / ITERATIONS;

    printf("  Create and join thread (empty):\n");
    printf("    Avg:   %.4f ms\n", avg);
    printf("    Min:   %.4f ms\n", minTime);
    printf("    Max:   %.4f ms\n", maxTime);
}

TEST(ThreadPoolBenchmark, ConcurrentTaskExecution)
{
    printf("\n=== Concurrent Task Execution ===\n");

    constexpr int NUM_THREADS = 4;
    constexpr int TASKS_PER_THREAD = 25;
    constexpr int TOTAL_TASKS = NUM_THREADS * TASKS_PER_THREAD;

    double sequentialTime = 0.0;
    for (int iter = 0; iter < 5; ++iter) {
        double start = GetTimeMs();
        for (int i = 0; i < TOTAL_TASKS; ++i) {
            volatile int counter = 0;
            for (int j = 0; j < 1000; ++j) {
                counter++;
            }
        }
        double end = GetTimeMs();
        sequentialTime += (end - start);
    }
    sequentialTime /= 5.0;

    double parallelTime = 0.0;
    for (int iter = 0; iter < 5; ++iter) {
        double start = GetTimeMs();
        std::vector<std::thread> threads;
        threads.reserve(NUM_THREADS);
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([]() {
                volatile int counter = 0;
                for (int j = 0; j < TASKS_PER_THREAD * 1000; ++j) {
                    counter++;
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        double end = GetTimeMs();
        parallelTime += (end - start);
    }
    parallelTime /= 5.0;

    printf("  Single thread (baseline): %.4f ms\n", sequentialTime);
    printf("  4 threads concurrent:    %.4f ms\n", parallelTime);
    printf("  Speedup: %.2fx\n", sequentialTime / parallelTime);
}

TEST(ThreadPoolBenchmark, LockContention)
{
    printf("\n=== Lock Contention ===\n");

    constexpr int NUM_THREADS = 4;
    constexpr int INCREMENTS = 10000;

    std::atomic<int> counter{0};

    double time = 0.0;
    for (int iter = 0; iter < 5; ++iter) {
        counter.store(0);
        double start = GetTimeMs();
        std::vector<std::thread> threads;
        threads.reserve(NUM_THREADS);
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&counter, INCREMENTS]() {
                for (int i = 0; i < INCREMENTS; ++i) {
                    counter.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        double end = GetTimeMs();
        time += (end - start);
    }
    time /= 5.0;

    printf("  4 threads, 10000 atomic increments: %.4f ms\n", time);
    printf("  Final counter value: %d (expected: %d)\n", counter.load(), NUM_THREADS * INCREMENTS);
}

TEST(ThreadPoolBenchmark, CPUCachePerformance)
{
    printf("\n=== CPU Cache Performance ===\n");

    constexpr int ARRAY_SIZE = 1024 * 1024;
    constexpr int NUM_ITERATIONS = 10;

    std::vector<int> arr(ARRAY_SIZE, 1);

    double seqTime = 0.0;
    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        long long sum = 0;
        double start = GetTimeMs();
        for (int i = 0; i < ARRAY_SIZE; ++i) {
            sum += arr[i];
        }
        double end = GetTimeMs();
        seqTime += (end - start);
        (void)sum;
    }
    seqTime /= NUM_ITERATIONS;

    double strideTime = 0.0;
    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        long long sum = 0;
        double start = GetTimeMs();
        for (int i = 0; i < ARRAY_SIZE; i += 16) {
            sum += arr[i];
        }
        double end = GetTimeMs();
        strideTime += (end - start);
        (void)sum;
    }
    strideTime /= NUM_ITERATIONS;

    printf("  Sequential access (cache-friendly): %.4f ms\n", seqTime);
    printf("  Stride-16 access (cache-unfriendly): %.4f ms\n", strideTime);
    printf("  Ratio: %.2fx\n", strideTime / seqTime);
}

TEST(ThreadPoolBenchmark, SleepPerformance)
{
    printf("\n=== Sleep Performance ===\n");

    constexpr int NUM_SLEEPS = 100;
    constexpr int SLEEP_DURATION_MS = 1;

    double time = 0.0;
    for (int iter = 0; iter < 5; ++iter) {
        double start = GetTimeMs();
        for (int i = 0; i < NUM_SLEEPS; ++i) {
            SleepMs(SLEEP_DURATION_MS);
        }
        double end = GetTimeMs();
        time += (end - start);
    }
    time /= 5.0;

    printf("  %d x sleep(%d ms): %.4f ms total (%.4f ms per sleep)\n",
           NUM_SLEEPS, SLEEP_DURATION_MS, time, time / NUM_SLEEPS);
}

TEST(ThreadPoolBenchmark, ThreadJoinOverhead)
{
    printf("\n=== Thread Join Overhead ===\n");

    constexpr int NUM_THREADS = 10;
    constexpr int ITERATIONS = 20;

    double time = 0.0;
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        std::vector<std::thread> threads;
        threads.reserve(NUM_THREADS);

        double start = GetTimeMs();
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back([]() {});
        }
        for (auto& t : threads) {
            t.join();
        }
        double end = GetTimeMs();
        time += (end - start);
    }
    time /= ITERATIONS;

    printf("  Create and join %d threads: %.4f ms\n", NUM_THREADS, time);
    printf("  Per thread: %.4f ms\n", time / NUM_THREADS);
}
