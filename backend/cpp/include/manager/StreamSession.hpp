#pragma once

#include <condition_variable>
#include <chrono>
#include <deque>
#include <mutex>
#include <optional>
#include <string>

#include "types/InterviewTypes.hpp"

// StreamSession 表示“一条正在流式输出的回答”。
// 它独立于 InterviewStreamManager，是因为 manager 只负责调度和限流，
// 而 session 负责保存这条流自己的状态、队列、取消标记和最终文本。
class StreamSession {
public:
    // 流生命周期状态：
    // Pending   任务已创建，但还没开始执行；
    // Running   provider 正在生成；
    // Done      正常结束；
    // Error     发生错误；
    // Cancelled 主动取消；
    // Expired   后续可用于超时清理。
    enum class State {
        Pending,
        Running,
        Done,
        Error,
        Cancelled,
        Expired
    };

    // 队列容量配置。
    // 当消费者写 SSE 的速度比 provider 慢时，这个上限可以防止队列
    // 无限增长；同时还能配合 chunk 合并减少抖动。
    struct Config {
        size_t maxQueueSize = 64;
    };

    StreamSession(std::string sessionId,
                  std::string threadId,
                  std::string messageId,
                  Config config);

    const std::string& sessionId() const;
    const std::string& threadId() const;
    const std::string& messageId() const;

    // 生产者入口：provider 线程把 chunk / done / error 事件推到这里。
    // 这个函数会做三件事：
    // 1. 检查是否已经结束或取消；
    // 2. 尽量把相邻 chunk 合并；
    // 3. 唤醒正在等数据的 SSE 线程。
    void push(const InterviewStreamEvent& event);

    // 消费者入口：SSE 线程在这里等待新事件。
    // 如果队列为空就阻塞；如果超时、关闭或取消，就返回 false。
    bool waitPop(InterviewStreamEvent& out, int timeoutMs);

    // 收尾控制：
    // close() 表示这个 session 已经不会再产生新事件；
    // cancel() 表示主动终止，并唤醒所有等待中的消费者。
    void close();
    void cancel();

    bool cancelled() const;
    bool closed() const;
    bool empty() const;
    size_t size() const;

    void setState(State state);
    State state() const;

    // 活动时间记录，用于后续超时和空闲清理。
    void touch();
    std::chrono::steady_clock::time_point lastActivity() const;

    // 取出完整的 assistant 文本，并清空内部缓存。
    // 这个结果会在流结束后写回数据库。
    std::string drainAssistantText();

    // 可选辅助函数：如果上层不想走事件队列，只想直接拼文本，
    // 可以用这个接口补充最终内容。
    void appendAssistantText(const std::string& delta);

private:
    bool pushChunkMerged(const std::string& chunk);

private:
    std::string m_sessionId;
    std::string m_threadId;
    std::string m_messageId;

    Config m_config;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;

    std::deque<InterviewStreamEvent> m_queue;

    bool m_closed = false;
    bool m_cancelled = false;
    State m_state = State::Pending;
    std::chrono::steady_clock::time_point m_lastActivity;
    std::string m_assistantText;
};
