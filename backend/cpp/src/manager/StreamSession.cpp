#include "manager/StreamSession.hpp"

#include <utility>

// 这个实现把 StreamSession 做成一个“线程安全的单流邮箱”。
// worker 线程负责 push 事件，SSE 输出线程负责 waitPop 消费事件。
// 所有可变状态都放在同一把 mutex 下，避免分散锁带来的复杂度。
StreamSession::StreamSession(std::string sessionId,
                             std::string threadId,
                             std::string messageId,
                             Config config)
    : m_sessionId(std::move(sessionId)),
      m_threadId(std::move(threadId)),
      m_messageId(std::move(messageId)),
      m_config(config),
      m_lastActivity(std::chrono::steady_clock::now()) {}

const std::string& StreamSession::sessionId() const { return m_sessionId; }
const std::string& StreamSession::threadId() const { return m_threadId; }
const std::string& StreamSession::messageId() const { return m_messageId; }

void StreamSession::setState(State state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = state;
}

StreamSession::State StreamSession::state() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

void StreamSession::touch() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastActivity = std::chrono::steady_clock::now();
}

std::chrono::steady_clock::time_point StreamSession::lastActivity() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastActivity;
}

bool StreamSession::cancelled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cancelled;
}

bool StreamSession::closed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_closed;
}

bool StreamSession::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

size_t StreamSession::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void StreamSession::appendAssistantText(const std::string& delta) {
    if (delta.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_assistantText += delta;
}

bool StreamSession::pushChunkMerged(const std::string& chunk) {
    if (chunk.empty()) {
        return false;
    }

    // 尝试把相邻 chunk 合并到队尾。
    // 这样前端看到的不是“一个 token 一个 token”地闪，而是更平滑的文本块。
    if (!m_queue.empty()) {
        auto& back = m_queue.back();
        if (back.type == InterviewStreamEventType::Chunk && back.content.has_value()) {
            back.content = back.content.value() + chunk;
            return true;
        }
    }

    if (m_queue.size() >= m_config.maxQueueSize) {
        return false;
    }

    InterviewStreamEvent event;
    event.type = InterviewStreamEventType::Chunk;
    event.content = chunk;
    m_queue.push_back(std::move(event));
    return true;
}

void StreamSession::push(const InterviewStreamEvent& event) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_closed || m_cancelled) {
        return;
    }

    m_lastActivity = std::chrono::steady_clock::now();

    if (event.type == InterviewStreamEventType::Chunk && event.content.has_value()) {
        m_assistantText += *event.content;

        // chunk 事件优先合并到尾部，减少队列长度并降低前端渲染抖动。
        if (!pushChunkMerged(*event.content)) {
            return;
        }

        m_cv.notify_one();
        return;
    }

    if (event.type == InterviewStreamEventType::Done) {
        m_state = State::Done;
        m_queue.push_back(event);
        m_closed = true;
        m_cv.notify_all();
        return;
    }

    if (event.type == InterviewStreamEventType::Error) {
        m_state = State::Error;
        m_queue.push_back(event);
        m_closed = true;
        m_cv.notify_all();
        return;
    }

    m_queue.push_back(event);
    m_cv.notify_one();
}

bool StreamSession::waitPop(InterviewStreamEvent& out, int timeoutMs) {
    std::unique_lock<std::mutex> lock(m_mutex);

    // 等待直到：
    // 1. 队列里有新事件；
    // 2. session 结束；
    // 3. session 被取消；
    // 4. 超时。
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

void StreamSession::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_closed = true;
    m_cv.notify_all();
}

void StreamSession::cancel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cancelled = true;
    m_state = State::Cancelled;
    m_cv.notify_all();
}

std::string StreamSession::drainAssistantText() {
    std::lock_guard<std::mutex> lock(m_mutex);
    // 一次性把累计文本取走并清空，避免后续重复落库。
    std::string result = std::move(m_assistantText);
    m_assistantText.clear();
    return result;
}
