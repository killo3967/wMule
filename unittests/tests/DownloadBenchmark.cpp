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
#include <vector>
#include <cstring>
#include <algorithm>
#include <chrono>

using namespace muleunit;

namespace {

double GetTimeMs() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double, std::milli>(duration).count();
}

}

DECLARE_SIMPLE(DownloadBenchmark);

TEST(DownloadBenchmark, PacketProcessingBaseline)
{
    printf("\n=== Packet Processing Baseline ===\n");

    constexpr int NUM_PACKETS = 1000;
    constexpr int PACKET_SIZE = 1024;
    constexpr int ITERATIONS = 100;

    std::vector<unsigned char> packet(PACKET_SIZE, 0xAA);

    double totalTime = 0.0;
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        double start = GetTimeMs();

        for (int i = 0; i < NUM_PACKETS; ++i) {
            volatile unsigned char sum = 0;
            for (int j = 0; j < PACKET_SIZE; ++j) {
                sum += packet[j];
            }
            (void)sum;
        }

        double end = GetTimeMs();
        totalTime += (end - start);
    }

    double avgTime = totalTime / ITERATIONS;
    double throughput = (NUM_PACKETS * PACKET_SIZE) / avgTime / 1024.0;

    printf("  Processed %d packets x %d bytes:\n", NUM_PACKETS, PACKET_SIZE);
    printf("    Time: %.4f ms\n", avgTime);
    printf("    Throughput: %.2f KB/s\n", throughput);
}

TEST(DownloadBenchmark, BufferCopyPerformance)
{
    printf("\n=== Buffer Copy Performance ===\n");

    constexpr int BUFFER_SIZE = 10 * 1024;  // 10KB
    constexpr int NUM_COPIES = 1000;
    constexpr int ITERATIONS = 20;

    std::vector<unsigned char> src(BUFFER_SIZE, 0xBB);
    std::vector<unsigned char> dst(BUFFER_SIZE, 0x00);

    double totalTime = 0.0;
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        double start = GetTimeMs();

        for (int i = 0; i < NUM_COPIES; ++i) {
            std::memcpy(dst.data(), src.data(), BUFFER_SIZE);
        }

        double end = GetTimeMs();
        totalTime += (end - start);
    }

    double avgTime = totalTime / ITERATIONS;
    double throughput = (NUM_COPIES * BUFFER_SIZE) / avgTime / 1024.0;

    printf("  Copied %d x %d KB buffers:\n", NUM_COPIES, BUFFER_SIZE / 1024);
    printf("    Time: %.4f ms\n", avgTime);
    printf("    Throughput: %.2f MB/s\n", throughput / 1024.0);
}

TEST(DownloadBenchmark, HashComputation)
{
    printf("\n=== Hash Computation Performance ===\n");

    constexpr int DATA_SIZE = 64 * 1024;  // 64KB
    constexpr int NUM_HASHES = 100;
    constexpr int ITERATIONS = 10;

    std::vector<unsigned char> data(DATA_SIZE);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<unsigned char>(i & 0xFF);
    }

    double totalTime = 0.0;
    for (int iter = 0; iter < ITERATIONS; ++iter) {
        double start = GetTimeMs();

        for (int i = 0; i < NUM_HASHES; ++i) {
            uint32_t hash = 0xFFFFFFFF;
            for (size_t j = 0; j < data.size(); ++j) {
                hash ^= data[j];
                hash *= 0x9E3779B9;
            }
            (void)hash;
        }

        double end = GetTimeMs();
        totalTime += (end - start);
    }

    double avgTime = totalTime / ITERATIONS;
    double throughput = (NUM_HASHES * DATA_SIZE) / avgTime / 1024.0 / 1024.0;

    printf("  Computed %d hashes x %d KB:\n", NUM_HASHES, DATA_SIZE / 1024);
    printf("    Time: %.4f ms\n", avgTime);
    printf("    Throughput: %.2f MB/s\n", throughput);
}

TEST(DownloadBenchmark, QueueOperations)
{
    printf("\n=== Queue Operations Performance ===\n");

    constexpr int QUEUE_SIZE = 10000;
    constexpr int OPERATIONS = 10000;
    constexpr int ITERATIONS = 5;

    std::vector<int> queue;
    queue.reserve(QUEUE_SIZE);

    double totalPushTime = 0.0;
    double totalPopTime = 0.0;

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        queue.clear();

        double startPush = GetTimeMs();
        for (int i = 0; i < OPERATIONS; ++i) {
            queue.push_back(i);
        }
        double endPush = GetTimeMs();
        totalPushTime += (endPush - startPush);

        double startPop = GetTimeMs();
        for (int i = 0; i < OPERATIONS; ++i) {
            volatile int val = queue.back();
            queue.pop_back();
            (void)val;
        }
        double endPop = GetTimeMs();
        totalPopTime += (endPop - startPop);
    }

    double avgPushTime = totalPushTime / ITERATIONS;
    double avgPopTime = totalPopTime / ITERATIONS;

    printf("  %d push/pop operations:\n", OPERATIONS);
    printf("    Push time: %.4f ms (%.2f ops/ms)\n", avgPushTime, OPERATIONS / avgPushTime);
    printf("    Pop time:  %.4f ms (%.2f ops/ms)\n", avgPopTime, OPERATIONS / avgPopTime);
}

TEST(DownloadBenchmark, SearchPerformance)
{
    printf("\n=== Search Performance ===\n");

    constexpr int ARRAY_SIZE = 10000;
    constexpr int SEARCHES = 1000;
    constexpr int ITERATIONS = 5;

    std::vector<int> arr(ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; ++i) {
        arr[i] = i;
    }
    std::sort(arr.begin(), arr.end());

    double totalLinearTime = 0.0;
    double totalBinaryTime = 0.0;

    for (int iter = 0; iter < ITERATIONS; ++iter) {
        double startLinear = GetTimeMs();
        for (int i = 0; i < SEARCHES; ++i) {
            int target = (i * 17) % ARRAY_SIZE;
            volatile bool found = false;
            for (int j = 0; j < ARRAY_SIZE; ++j) {
                if (arr[j] == target) {
                    found = true;
                    break;
                }
            }
            (void)found;
        }
        double endLinear = GetTimeMs();
        totalLinearTime += (endLinear - startLinear);

        double startBinary = GetTimeMs();
        for (int i = 0; i < SEARCHES; ++i) {
            int target = (i * 17) % ARRAY_SIZE;
            volatile bool found = std::binary_search(arr.begin(), arr.end(), target);
            (void)found;
        }
        double endBinary = GetTimeMs();
        totalBinaryTime += (endBinary - startBinary);
    }

    double avgLinearTime = totalLinearTime / ITERATIONS;
    double avgBinaryTime = totalBinaryTime / ITERATIONS;

    printf("  %d searches in %d element array:\n", SEARCHES, ARRAY_SIZE);
    printf("    Linear search:  %.4f ms\n", avgLinearTime);
    printf("    Binary search: %.6f ms\n", avgBinaryTime);
    printf("    Speedup: %.0fx\n", avgLinearTime / avgBinaryTime);
}
