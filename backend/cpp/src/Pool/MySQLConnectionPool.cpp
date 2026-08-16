#include "Pool/MySQLConnectionPool.hpp"
#include <stdexcept>
#include <chrono>

MySQLConnHandle::MySQLConnHandle(MySQLConnectionPool* pool, MySQLConn* conn)
    : m_pool(pool), m_conn(conn) {}

MySQLConnHandle::~MySQLConnHandle() {
    // 句柄离开作用域时自动归还连接，保证异常路径也不会泄漏连接。
    if (m_pool && m_conn) {
        m_pool->release(m_conn);
    }
}

MySQLConnHandle::MySQLConnHandle(MySQLConnHandle&& other) noexcept
    : m_pool(other.m_pool), m_conn(other.m_conn) {
    other.m_pool = nullptr;
    other.m_conn = nullptr;
}

MySQLConnHandle& MySQLConnHandle::operator=(MySQLConnHandle&& other) noexcept {
    if (this != &other) {
        if (m_pool && m_conn) {
            m_pool->release(m_conn);
        }
        m_pool = other.m_pool;
        m_conn = other.m_conn;
        other.m_pool = nullptr;
        other.m_conn = nullptr;
    }
    return *this;
}

MySQLConnectionPool::MySQLConnectionPool(const MySQLPoolConfig& cfg)
    : m_cfg(cfg) {
    // 配置必须合法，避免出现“最小连接数大于最大连接数”这类逻辑错误。
    if (m_cfg.minSize == 0 || m_cfg.maxSize == 0 || m_cfg.minSize > m_cfg.maxSize) {
        throw std::invalid_argument("invalid mysql pool size");
    }

    // 先预热 minSize 个连接，减少第一批请求的等待时间。
    for (size_t i = 0; i < m_cfg.minSize; ++i) {
        MySQLConn* conn = createRawConn();
        if (conn) {
            m_idle.push(conn);
            ++m_total;
        }
    }
}

MySQLConnectionPool::~MySQLConnectionPool() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdown = true;
        destroyIdleLocked();
    }
    m_cv.notify_all();
}

MySQLConn* MySQLConnectionPool::createRawConn() {
    // 连接池统一管理连接生命周期，所以这里创建原始指针，
    // 成功后交给池内部保存，失败则返回 nullptr。
    auto* conn = new MySQLConn();
    if (!conn->connect(m_cfg.user, m_cfg.password, m_cfg.dbName, m_cfg.host, m_cfg.port)) {
        delete conn;
        return nullptr;
    }
    conn->refreshAlivetime();
    return conn;
}

MySQLConnHandle MySQLConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(m_mutex);
    ++m_stats.acquireRequests;

    // 借连接不是无限等待，必须在 deadline 前拿到或返回空句柄。
    const auto deadline = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(m_cfg.acquireTimeoutMs);

    while (true) {
        if (m_shutdown) {
            return {};
        }

        if (!m_idle.empty()) {
            // 优先复用空闲连接。
            MySQLConn* conn = m_idle.front();
            m_idle.pop();
            ++m_busy;
            ++m_stats.idleHits;
            conn->refreshAlivetime();
            return MySQLConnHandle(this, conn);
        }

        if (m_total < m_cfg.maxSize) {
            // 没有空闲连接时允许扩容，但建连过程在锁外完成，
            // 避免把整个池阻塞在 I/O 上。
            ++m_total;
            lock.unlock();

            MySQLConn* conn = createRawConn();

            lock.lock();
            if (m_shutdown) {
                --m_total;
                delete conn;
                return {};
            }

            if (!conn) {
                --m_total;
                ++m_stats.failedConnections;
                return {};
            }

            ++m_busy;
            ++m_stats.newConnections;
            return MySQLConnHandle(this, conn);
        }

        if (m_cv.wait_until(lock, deadline) == std::cv_status::timeout) {
            // 超时后让调用方自己决定是失败返回还是重试。
            ++m_stats.acquireTimeouts;
            return {};
        }
    }
}

void MySQLConnectionPool::release(MySQLConn* conn) {
    if (!conn) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_shutdown) {
        // 池关闭后不再回收连接，直接销毁。
        delete conn;
        return;
    }

    // 归还前刷新活跃时间，便于后续做空闲连接管理。
    conn->refreshAlivetime();
    m_idle.push(conn);
    --m_busy;
    m_cv.notify_one();
}

size_t MySQLConnectionPool::idleSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_idle.size();
}

size_t MySQLConnectionPool::totalSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_total;
}

size_t MySQLConnectionPool::busySize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_busy;
}

void MySQLConnectionPool::destroyIdleLocked() {
    while (!m_idle.empty()) {
        delete m_idle.front();
        m_idle.pop();
    }
}

MySQLPoolStats MySQLConnectionPool::stats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}
