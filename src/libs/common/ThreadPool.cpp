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

#include "ThreadPool.h"
#include <algorithm>

CThreadPool& CThreadPool::Instance() {
    static CThreadPool instance;
    return instance;
}

CThreadPool::CThreadPool() {
    size_t hwConcurrency = std::thread::hardware_concurrency();
    if (hwConcurrency == 0) {
        hwConcurrency = 4;
    }
    
    m_minThreads = std::min(size_t(2), hwConcurrency);
    m_maxThreads = std::min(size_t(8), hwConcurrency);

    for (size_t i = 0; i < m_minThreads; ++i) {
        CreateWorker();
    }
}

CThreadPool::~CThreadPool() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_cv.notify_all();
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void CThreadPool::CreateWorker() {
    if (m_workers.size() < m_maxThreads) {
        m_workers.emplace_back(&CThreadPool::WorkerThread, this);
    }
}

void CThreadPool::WorkerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] {
                return m_stop || !m_tasks.empty();
            });

            if (m_stop && m_tasks.empty()) {
                return;
            }

            if (!m_tasks.empty()) {
                task = std::move(m_tasks.front());
                m_tasks.pop();
                ++m_activeTasks;
            }
        }

        if (task) {
            task();
            --m_activeTasks;
            m_complete.notify_all();
        }
    }
}

Threading::DrainResult CThreadPool::Drain(std::chrono::milliseconds timeout)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	Threading::DrainResult result = Threading::DrainResult::Completed;
	const auto emptyPredicate = [this]() {
		return m_tasks.empty() && m_activeTasks == 0;
	};
	if (timeout.count() <= 0) {
		m_complete.wait(lock, emptyPredicate);
	} else if (!m_complete.wait_for(lock, timeout, emptyPredicate)) {
		result = Threading::DrainResult::TimedOut;
	}
	Threading::ThreadMetrics::Instance().RegisterDrainResult(result);
	return result;
}

void CThreadPool::SetThreadCount(size_t count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t newCount = std::max(m_minThreads, std::min(count, m_maxThreads));
    
    while (m_workers.size() < newCount) {
        m_workers.emplace_back(&CThreadPool::WorkerThread, this);
    }
}
