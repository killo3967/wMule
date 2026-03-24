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

#include "AsyncSocket.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#endif

namespace amule {
namespace async {

static bool s_initialized = false;

bool AsyncSocket::Initialize() {
    if (s_initialized) {
        return true;
    }
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        return false;
    }
#endif
    s_initialized = true;
    return true;
}

bool AsyncSocket::Cleanup() {
    if (!s_initialized) {
        return true;
    }
#ifdef _WIN32
    WSACleanup();
#endif
    s_initialized = false;
    return true;
}

AsyncSocket::AsyncSocket()
    : m_handle(nullptr)
    , m_connected(false)
    , m_reading(false)
    , m_lastError(ErrorSuccess)
    , m_nonBlocking(false)
{
#ifdef _WIN32
    m_handle = reinterpret_cast<void*>(INVALID_SOCKET);
#else
    m_handle = nullptr;
#endif
}

AsyncSocket::~AsyncSocket() {
    Close();
}

AsyncSocket::AsyncSocket(AsyncSocket&& other) noexcept
    : m_handle(other.m_handle)
    , m_connected(other.m_connected.load())
    , m_reading(other.m_reading.load())
    , m_lastError(other.m_lastError)
    , m_onRead(std::move(other.m_onRead))
    , m_onWrite(std::move(other.m_onWrite))
    , m_onConnect(std::move(other.m_onConnect))
    , m_nonBlocking(other.m_nonBlocking.load())
{
#ifdef _WIN32
    other.m_handle = reinterpret_cast<void*>(INVALID_SOCKET);
#else
    other.m_handle = nullptr;
#endif
    other.m_connected = false;
}

AsyncSocket& AsyncSocket::operator=(AsyncSocket&& other) noexcept {
    if (this != &other) {
        Close();
        m_handle = other.m_handle;
        m_connected = other.m_connected.load();
        m_reading = other.m_reading.load();
        m_lastError = other.m_lastError;
        m_onRead = std::move(other.m_onRead);
        m_onWrite = std::move(other.m_onWrite);
        m_onConnect = std::move(other.m_onConnect);
        m_nonBlocking = other.m_nonBlocking.load();
#ifdef _WIN32
        other.m_handle = reinterpret_cast<void*>(INVALID_SOCKET);
#else
        other.m_handle = nullptr;
#endif
        other.m_connected = false;
    }
    return *this;
}

#ifdef _WIN32
using NativeSocket = SOCKET;
#else
using NativeSocket = int;
constexpr NativeSocket INVALID_SOCKET_VALUE = -1;
#endif

static NativeSocket GetNativeSocket(void* handle) {
#ifdef _WIN32
    return reinterpret_cast<SOCKET>(handle);
#else
    return static_cast<int>(reinterpret_cast<intptr_t>(handle));
#endif
}

static void* CreateHandle(NativeSocket sock) {
#ifdef _WIN32
    return reinterpret_cast<void*>(sock);
#else
    return reinterpret_cast<void*>(static_cast<intptr_t>(sock));
#endif
}

#ifdef _WIN32
constexpr NativeSocket INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
constexpr NativeSocket INVALID_SOCKET_VALUE = -1;
#endif

bool AsyncSocket::Connect(const char* host, uint16_t port, ConnectCallback callback) {
    NativeSocket sock = GetNativeSocket(m_handle);
    if (sock != INVALID_SOCKET && sock != INVALID_SOCKET_VALUE) {
        Close();
    }
    
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET || sock == INVALID_SOCKET_VALUE) {
        m_lastError = ErrorUnknown;
        return false;
    }
    
    m_handle = CreateHandle(sock);
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &serverAddr.sin_addr) <= 0) {
        struct hostent* he = gethostbyname(host);
        if (he == nullptr) {
            m_lastError = ErrorUnknown;
            Close();
            return false;
        }
        memcpy(&serverAddr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    SetNonBlocking(true);
    
    int result = connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    if (result == SOCKET_ERROR) {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS) {
            m_onConnect = callback;
            return true;
        }
#else
        if (errno == EINPROGRESS) {
            m_onConnect = callback;
            return true;
        }
#endif
        m_lastError = ErrorUnknown;
        Close();
        return false;
    }
    
    m_connected = true;
    if (callback) {
        callback(true, ErrorSuccess);
    }
    return true;
}

void AsyncSocket::AsyncRead(uint8_t* buffer, size_t size, ReadCallback callback) {
    NativeSocket sock = GetNativeSocket(m_handle);
    if (sock == INVALID_SOCKET || sock == INVALID_SOCKET_VALUE || !m_connected) {
        if (callback) {
            callback(0, ErrorDisconnected);
        }
        return;
    }
    
    m_onRead = callback;
    m_reading = true;
    
    int bytesRead = recv(sock, reinterpret_cast<char*>(buffer), static_cast<int>(size), 0);
    
    m_reading = false;
    
    if (bytesRead == SOCKET_ERROR) {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            m_lastError = ErrorWouldBlock;
        } else {
            m_lastError = ErrorUnknown;
            m_connected = false;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            m_lastError = ErrorWouldBlock;
        } else {
            m_lastError = ErrorUnknown;
            m_connected = false;
        }
#endif
        if (callback) {
            callback(0, m_lastError);
        }
        return;
    }
    
    if (bytesRead == 0) {
        m_connected = false;
        if (callback) {
            callback(0, ErrorDisconnected);
        }
        return;
    }
    
    if (callback) {
        callback(static_cast<size_t>(bytesRead), ErrorSuccess);
    }
}

void AsyncSocket::AsyncWrite(const uint8_t* buffer, size_t size, WriteCallback callback) {
    NativeSocket sock = GetNativeSocket(m_handle);
    if (sock == INVALID_SOCKET || sock == INVALID_SOCKET_VALUE || !m_connected) {
        if (callback) {
            callback(0, ErrorDisconnected);
        }
        return;
    }
    
    m_onWrite = callback;
    
    int bytesSent = send(sock, reinterpret_cast<const char*>(buffer), static_cast<int>(size), 0);
    
    if (bytesSent == SOCKET_ERROR) {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) {
            m_lastError = ErrorWouldBlock;
        } else {
            m_lastError = ErrorUnknown;
            m_connected = false;
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            m_lastError = ErrorWouldBlock;
        } else {
            m_lastError = ErrorUnknown;
            m_connected = false;
        }
#endif
        if (callback) {
            callback(0, m_lastError);
        }
        return;
    }
    
    if (callback) {
        callback(static_cast<size_t>(bytesSent), ErrorSuccess);
    }
}

void AsyncSocket::Close() {
    NativeSocket sock = GetNativeSocket(m_handle);
    if (sock != INVALID_SOCKET && sock != INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
#ifdef _WIN32
        m_handle = reinterpret_cast<void*>(INVALID_SOCKET);
#else
        m_handle = nullptr;
#endif
    }
    m_connected = false;
    m_reading = false;
}

bool AsyncSocket::SetNonBlocking(bool enable) {
    NativeSocket sock = GetNativeSocket(m_handle);
    if (sock == INVALID_SOCKET || sock == INVALID_SOCKET_VALUE) {
        return false;
    }
    
#ifdef _WIN32
    u_long mode = enable ? 1 : 0;
    if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
        return false;
    }
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    if (enable) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(sock, F_SETFL, flags) == -1) {
        return false;
    }
#endif
    
    m_nonBlocking = enable;
    return true;
}

SocketPool& SocketPool::Instance() {
    static SocketPool instance;
    return instance;
}

SocketPool::SocketPool()
    : m_activeCount(0)
    , m_poolSize(4)
{
    for (size_t i = 0; i < m_poolSize; ++i) {
        m_pool.emplace_back(std::make_unique<AsyncSocket>());
    }
}

SocketPool::~SocketPool() {
    m_pool.clear();
}

AsyncSocket* SocketPool::Acquire() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& socket : m_pool) {
        if (!socket->IsConnected()) {
            m_activeCount++;
            return socket.get();
        }
    }
    
    if (m_pool.size() < m_poolSize * 2) {
        auto socket = std::make_unique<AsyncSocket>();
        AsyncSocket* ptr = socket.get();
        m_pool.push_back(std::move(socket));
        m_activeCount++;
        return ptr;
    }
    
    return nullptr;
}

void SocketPool::Release(AsyncSocket* socket) {
    if (socket) {
        socket->Close();
        m_activeCount--;
    }
}

void SocketPool::SetPoolSize(size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_poolSize = size;
    
    while (m_pool.size() > size) {
        m_pool.pop_back();
    }
    
    while (m_pool.size() < size) {
        m_pool.emplace_back(std::make_unique<AsyncSocket>());
    }
}

} // namespace async
} // namespace amule
