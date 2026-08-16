#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <string>

#include "MySQLConn.hpp"

// MySQL 连接池配置。
// minSize/maxSize 决定池的初始连接数和最大连接数；
// acquireTimeoutMs 决定借连接时的等待超时。
struct MySQLPoolConfig {
    std::string host;
    std::string user;
    std::string password;
    std::string dbName;
    unsigned short port = 3306;
    size_t minSize = 2;
    size_t maxSize = 10;
    size_t acquireTimeoutMs = 3000;
};

struct MySQLPoolStats {
    size_t acquireRequests = 0;
    size_t idleHits = 0;
    size_t newConnections = 0;
    size_t acquireTimeouts = 0;
    size_t failedConnections = 0;
};

class MySQLConnectionPool;

// RAII 句柄。
// 调用 acquire() 后拿到的不是裸连接，而是这个句柄。
// 句柄析构时会自动把连接归还到池中，避免忘记 release。
class MySQLConnHandle {
public:
    MySQLConnHandle() = default;
    ~MySQLConnHandle();

    MySQLConnHandle(MySQLConnHandle&& other) noexcept;
    MySQLConnHandle& operator=(MySQLConnHandle&& other) noexcept;

    MySQLConnHandle(const MySQLConnHandle&) = delete;
    MySQLConnHandle& operator=(const MySQLConnHandle&) = delete;

    MySQLConn* operator->() const { return m_conn; }
    MySQLConn& operator*() const { return *m_conn; }
    explicit operator bool() const { return m_conn != nullptr; }

private:
    friend class MySQLConnectionPool;
    MySQLConnHandle(MySQLConnectionPool* pool, MySQLConn* conn);

    MySQLConnectionPool* m_pool = nullptr;
    MySQLConn* m_conn = nullptr;
};

// 线程安全的 MySQL 连接池。
// 目标是复用连接、控制并发借还、减少频繁建连/断连的开销。
class MySQLConnectionPool {
public:
    explicit MySQLConnectionPool(const MySQLPoolConfig& cfg);
    ~MySQLConnectionPool();

    // 借出一个连接。
    // 如果有空闲连接，直接复用；
    // 如果总连接数未达到上限，则会尝试新建；
    // 如果都不满足，就等待直到超时。
    MySQLConnHandle acquire();

    // 这三个接口主要用于监控和调试。
    size_t idleSize() const;
    size_t totalSize() const;
    size_t busySize() const;
    MySQLPoolStats stats() const;

private:
    friend class MySQLConnHandle;
    MySQLConn* createRawConn();
    void release(MySQLConn* conn);
    void destroyIdleLocked();

private:
    MySQLPoolConfig m_cfg;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<MySQLConn*> m_idle;
    size_t m_total = 0;
    size_t m_busy = 0;
    bool m_shutdown = false;
    MySQLPoolStats m_stats;
};
