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

#ifndef AMULE_ASYNCSOCKET_H
#define AMULE_ASYNCSOCKET_H

#include <cstdint>
#include <cstddef>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace amule {
namespace async {

using ErrorCode = int;

constexpr ErrorCode ErrorSuccess = 0;
constexpr ErrorCode ErrorWouldBlock = -1;
constexpr ErrorCode ErrorDisconnected = -2;
constexpr ErrorCode ErrorTimeout = -3;
constexpr ErrorCode ErrorUnknown = -99;

class AsyncSocket {
public:
    using ReadCallback = std::function<void(size_t bytesRead, ErrorCode error)>;
    using WriteCallback = std::function<void(size_t bytesWritten, ErrorCode error)>;
    using ConnectCallback = std::function<void(bool success, ErrorCode error)>;

    AsyncSocket();
    ~AsyncSocket();

    AsyncSocket(const AsyncSocket&) = delete;
    AsyncSocket& operator=(const AsyncSocket&) = delete;

    AsyncSocket(AsyncSocket&& other) noexcept;
    AsyncSocket& operator=(AsyncSocket&& other) noexcept;

    bool Connect(const char* host, uint16_t port, ConnectCallback callback = nullptr);
    void AsyncRead(uint8_t* buffer, size_t size, ReadCallback callback = nullptr);
    void AsyncWrite(const uint8_t* buffer, size_t size, WriteCallback callback = nullptr);
    
    bool IsConnected() const { return m_connected.load(); }
    void Close();
    
    void* GetHandle() const { return m_handle; }
    ErrorCode GetLastError() const { return m_lastError; }
    
    static bool Initialize();
    static bool Cleanup();

private:
    void* m_handle;
    std::atomic<bool> m_connected;
    std::atomic<bool> m_reading;
    ErrorCode m_lastError;
    
    ReadCallback m_onRead;
    WriteCallback m_onWrite;
    ConnectCallback m_onConnect;
    
    std::mutex m_mutex;
    
    bool SetNonBlocking(bool enable);
    std::atomic<bool> m_nonBlocking{false};
};

class AsyncBuffer {
public:
    AsyncBuffer() : m_data(), m_offset(0) {}
    explicit AsyncBuffer(size_t capacity) : m_data(capacity), m_offset(0) {}
    
    uint8_t* data() { return m_data.data(); }
    const uint8_t* data() const { return m_data.data(); }
    size_t size() const { return m_data.size() - m_offset; }
    size_t capacity() const { return m_data.capacity(); }
    size_t totalSize() const { return m_data.size(); }
    
    void append(const uint8_t* data, size_t size) {
        m_data.insert(m_data.end(), data, data + size);
    }
    
    void consume(size_t bytes) {
        m_offset += bytes;
        if (m_offset >= m_data.size()) {
            clear();
        }
    }
    
    void clear() {
        m_data.clear();
        m_offset = 0;
    }
    
    void compact() {
        if (m_offset > 0) {
            m_data.erase(m_data.begin(), m_data.begin() + m_offset);
            m_offset = 0;
        }
    }
    
    const uint8_t* readPtr() const { return m_data.data() + m_offset; }

private:
    std::vector<uint8_t> m_data;
    size_t m_offset;
};

class SocketPool {
public:
    static SocketPool& Instance();
    
    AsyncSocket* Acquire();
    void Release(AsyncSocket* socket);
    size_t GetActiveCount() const { return m_activeCount.load(); }
    size_t GetPoolSize() const { return m_poolSize; }
    
    void SetPoolSize(size_t size);
    
private:
    SocketPool();
    ~SocketPool();
    
    std::vector<std::unique_ptr<AsyncSocket>> m_pool;
    std::atomic<size_t> m_activeCount;
    size_t m_poolSize;
    std::mutex m_mutex;
};

} // namespace async
} // namespace amule

#endif // AMULE_ASYNCSOCKET_H
