#include "manager/BoundedStreamEventQueuer.hpp"

#include <chrono>
#include <utility>

BoundedStreamEventQueuer::BoundedStreamEventQueuer()
    : BoundedStreamEventQueuer(Config{}) {}

BoundedStreamEventQueuer::BoundedStreamEventQueuer(Config config)
    : m_config(config) {}

bool BoundedStreamEventQueuer::closed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_closed;
}

bool BoundedStreamEventQueuer::cancelled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cancelled;
}

bool BoundedStreamEventQueuer::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

size_t BoundedStreamEventQueuer::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

bool BoundedStreamEventQueuer::mergeChunkToTail(const std::string& chunk) {
    if (chunk.empty()) {
        return false;
    }

    // 如果队尾已经是 chunk，就直接拼接，减少队列膨胀。
    if (!m_queue.empty()) {
        auto& back = m_queue.back();
        if (back.type == InterviewStreamEventType::Chunk && back.content.has_value()) {
            back.content = back.content.value() + chunk;
            return true;
        }
    }

    // 队列已满时，拒绝继续塞入低价值中间态，保护内存。
    if (m_queue.size() >= m_config.maxSize) {
        return false;
    }

    InterviewStreamEvent event;
    event.type = InterviewStreamEventType::Chunk;
    event.content = chunk;
    m_queue.push_back(std::move(event));
    return true;
}

bool BoundedStreamEventQueuer::push(const InterviewStreamEvent& event) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_closed || m_cancelled) {
        return false;
    }

    // chunk 优先合并，done/error 强制入队。
    if (event.type == InterviewStreamEventType::Chunk && event.content.has_value()) {
        const bool ok = mergeChunkToTail(*event.content);
        if (ok) {
            m_cv.notify_one();
        }
        return ok;
    }

    if (event.type == InterviewStreamEventType::Done ||
        event.type == InterviewStreamEventType::Error) {
        if (m_queue.size() >= m_config.maxSize) {
            // 终态事件不能静默丢掉，否则消费者会永久挂住。
            // 如果队列满了，优先把尾部一个 chunk 丢掉，腾位置给终态。
            if (!m_queue.empty()) {
                m_queue.pop_back();
            }
        }

        m_queue.push_back(event);
        m_closed = true;
        m_cv.notify_all();
        return true;
    }

    if (m_queue.size() >= m_config.maxSize) {
        return false;
    }

    m_queue.push_back(event);
    m_cv.notify_one();
    return true;
}

bool BoundedStreamEventQueuer::waitPop(InterviewStreamEvent& out, int timeoutMs) {
    std::unique_lock<std::mutex> lock(m_mutex);

    auto ready = [&]() {
        return !m_queue.empty() || m_closed || m_cancelled;
    };

    if (!m_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), ready)) {
        return false;
    }

    if (!m_queue.empty()) {
        out = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }

    return false;
}

void BoundedStreamEventQueuer::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_closed = true;
    m_cv.notify_all();
}

void BoundedStreamEventQueuer::cancel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cancelled = true;
    m_closed = true;
    m_cv.notify_all();
}
