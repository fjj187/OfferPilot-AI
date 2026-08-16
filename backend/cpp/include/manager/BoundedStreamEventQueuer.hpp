#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>

#include "types/InterviewTypes.hpp"

class BoundedStreamEventQueuer {
public:
    struct Config {
        size_t maxSize = 64;
    };

    BoundedStreamEventQueuer();
    explicit BoundedStreamEventQueuer(Config config);

    // 生产者入口：往队列里塞事件。
    // 返回 false 表示队列已关闭、已取消，或者当前事件被丢弃/合并失败。
    bool push(const InterviewStreamEvent& event);

    // 消费者入口：阻塞等待事件。
    // 超时返回 false；如果队列已关闭且已空，也返回 false。
    bool waitPop(InterviewStreamEvent& out, int timeoutMs);

    // 关闭队列。关闭后不会再接收新事件，但允许把已有事件消费完。
    void close();

    // 取消队列。取消后立即唤醒所有等待者，通常用于前端断开或超时。
    void cancel();

    bool closed() const;
    bool cancelled() const;
    bool empty() const;
    size_t size() const;

private:
    // 尝试把 chunk 合并到队尾，减少过细粒度的输出。
    bool mergeChunkToTail(const std::string& chunk);

private:
    Config m_config;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<InterviewStreamEvent> m_queue;

    bool m_closed = false;
    bool m_cancelled = false;
};
