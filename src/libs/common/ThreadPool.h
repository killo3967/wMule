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

#ifndef AMULE_THREADPOOL_H
#define AMULE_THREADPOOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>
#include <vector>
#include <chrono>

#include <common/threading/ThreadGuards.h>

class CThreadPool {
public:
    static CThreadPool& Instance();

    CThreadPool(const CThreadPool&) = delete;
    CThreadPool& operator=(const CThreadPool&) = delete;

    template<typename F>
    void Enqueue(F&& task, bool allowWhileShuttingDown = false) {
        if (!allowWhileShuttingDown && !Threading::ShutdownToken().GuardEnqueueOrErr("CThreadPool::Enqueue")) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.push(std::forward<F>(task));
        }
        m_cv.notify_one();
    }

    Threading::DrainResult Drain(std::chrono::milliseconds timeout);

    void WaitAll() {
        Drain(std::chrono::milliseconds(0));
    }

    size_t GetQueueSize() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tasks.size();
    }

    size_t GetThreadCount() const {
        return m_workers.size();
    }

    void SetThreadCount(size_t count);

private:
    CThreadPool();
    ~CThreadPool();

    void WorkerThread();
    void CreateWorker();

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::condition_variable m_complete;
    std::atomic<size_t> m_activeTasks{0};
    bool m_stop = false;

    size_t m_minThreads = 2;
    size_t m_maxThreads = 8;
};

template<typename F>
void ThreadPoolEnqueue(F&& task, bool allowWhileShuttingDown = false) {
	CThreadPool::Instance().Enqueue(std::forward<F>(task), allowWhileShuttingDown);
}

#endif // AMULE_THREADPOOL_H
