#include "manager/InterviewStreamManager.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <exception>
#include <utility>

namespace {
constexpr size_t kChunkFlushThreshold = 24;

bool shouldFlushChunkBuffer(const std::string& buffer, const std::string& delta) {
    if (buffer.size() >= kChunkFlushThreshold) {
        return true;
    }

    for (unsigned char ch : delta) {
        if (std::isspace(ch) || ch == '.' || ch == '!' || ch == '?' || ch == ',' || ch == ';' || ch == ':') {
            return true;
        }
    }

    return false;
}
}

// StreamKeyHash 用于把 session/thread 组合键放进 unordered_map。
// 这里采用轻量级异或哈希，足够满足“按会话定位流对象”的需求。
size_t InterviewStreamManager::StreamKeyHash::operator()(const StreamKey& key) const noexcept {
    return std::hash<std::string>{}(key.sessionId) ^ (std::hash<std::string>{}(key.threadId) << 1);
}

InterviewStreamManager::InterviewStreamManager(
    InterviewProvider& provider,
    ISessionRepository& sessionRepo,
    Config config)
    : m_provider(provider),
      m_sessionRepo(sessionRepo),
      m_config(config) {
    startWorkerLoop();
}

InterviewStreamManager::~InterviewStreamManager() {
    stopWorkers();
}

std::shared_ptr<StreamSession> InterviewStreamManager::createSession(const InterviewStreamRequest& request) {
    auto session = std::make_shared<StreamSession>(
        request.sessionId,
        request.threadId,
        request.messageId,
        StreamSession::Config{m_config.maxQueueSizePerStream});

    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        StreamKey key{request.sessionId, request.threadId};
        m_sessions[key] = session;
    }

    return session;
}

std::shared_ptr<StreamSession> InterviewStreamManager::getSession(
    const std::string& sessionId,
    const std::string& threadId) {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    StreamKey key{sessionId, threadId};
    auto it = m_sessions.find(key);
    if (it == m_sessions.end()) {
        return nullptr;
    }
    return it->second;
}

InterviewStreamManager::StartResult InterviewStreamManager::submit(
    const InterviewStreamRequest& request,
    const std::shared_ptr<StreamSession>& session) {
    if (!session) {
        return {false, "session is null"};
    }

    // 先确认这条 session 已经登记到全局索引里，并把状态切到 Running。
    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        StreamKey key{request.sessionId, request.threadId};
        auto it = m_sessions.find(key);
        if (it == m_sessions.end()) {
            return {false, "session not found"};
        }

        if (it->second->state() == StreamSession::State::Running) {
            return {false, "stream already running for this session/thread"};
        }

        it->second->setState(StreamSession::State::Running);
    }

    // 先占一个全局并发名额，避免 worker 过多导致系统被打满。
    if (!tryAcquireSlot()) {
        { std::lock_guard<std::mutex> lock(m_statsMutex); ++m_stats.rejected; }
        session->setState(StreamSession::State::Error);

        InterviewStreamEvent event;
        event.type = InterviewStreamEventType::Error;
        event.error = ApiError{ApiErrorCode::InternalError, "max inflight streams reached"};
        session->push(event);

        return {false, "max inflight streams reached"};
    }

    {
        std::lock_guard<std::mutex> lock(m_tasksMutex);
        m_tasks.emplace_back([this, request, session]() {
            executeStream(request, session);
        });
        std::lock_guard<std::mutex> statsLock(m_statsMutex);
        ++m_stats.accepted;
        ++m_stats.queued;
        m_stats.peakQueued = std::max(m_stats.peakQueued, m_stats.queued);
    }
    m_tasksCv.notify_one();

    return {true, ""};
}

void InterviewStreamManager::cancel(const std::string& sessionId, const std::string& threadId) {
    auto session = getSession(sessionId, threadId);
    if (!session) {
        return;
    }

    // 取消时先通知 session 本身，让等待中的 SSE 线程尽快返回。
    session->cancel();
    { std::lock_guard<std::mutex> lock(m_statsMutex); ++m_stats.cancelled; }
    session->setState(StreamSession::State::Cancelled);

    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    StreamKey key{sessionId, threadId};
    auto it = m_sessions.find(key);
    if (it != m_sessions.end()) {
        it->second->setState(StreamSession::State::Cancelled);
    }
}

void InterviewStreamManager::clearFinished() {
    std::lock_guard<std::mutex> lock(m_sessionsMutex);

    for (auto it = m_sessions.begin(); it != m_sessions.end(); ) {
        const auto state = it->second->state();
        // 已结束的会话不再保留在内存索引里，避免长期运行后越积越多。
        if (state == StreamSession::State::Done ||
            state == StreamSession::State::Error ||
            state == StreamSession::State::Cancelled ||
            state == StreamSession::State::Expired) {
            it = m_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

bool InterviewStreamManager::tryAcquireSlot() {
    std::lock_guard<std::mutex> lock(m_slotsMutex);
    if (m_inflightStreams >= m_config.maxInflightStreams) {
        return false;
    }
    ++m_inflightStreams;
    { std::lock_guard<std::mutex> statsLock(m_statsMutex); ++m_stats.active; m_stats.peakActive = std::max(m_stats.peakActive, m_stats.active); }
    return true;
}

void InterviewStreamManager::releaseSlot() {
    std::lock_guard<std::mutex> lock(m_slotsMutex);
    if (m_inflightStreams > 0) {
        --m_inflightStreams;
    }
    { std::lock_guard<std::mutex> statsLock(m_statsMutex); if (m_stats.active > 0) --m_stats.active; ++m_stats.completed; }
}

void InterviewStreamManager::startWorkerLoop() {
    m_stopping = false;
    const size_t count = std::max<size_t>(1, m_config.workerCount);

    // worker 只负责从任务队列取任务并执行，不参与业务决策。
    for (size_t i = 0; i < count; ++i) {
        m_workers.emplace_back([this]() {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(m_tasksMutex);
                    m_tasksCv.wait(lock, [&]() {
                        return m_stopping || !m_tasks.empty();
                    });

                    if (m_stopping && m_tasks.empty()) {
                        return;
                    }

                    task = std::move(m_tasks.front());
                    m_tasks.pop_front();
                    std::lock_guard<std::mutex> statsLock(m_statsMutex);
                    if (m_stats.queued > 0) --m_stats.queued;
                }

                if (task) {
                    task();
                }
            }
        });
    }
}

void InterviewStreamManager::stopWorkers() {
    {
        std::lock_guard<std::mutex> lock(m_tasksMutex);
        m_stopping = true;
    }
    m_tasksCv.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
}

void InterviewStreamManager::executeStream(
    InterviewStreamRequest request,
    std::shared_ptr<StreamSession> session) {
    auto context = std::make_shared<ProviderContext>();
    context->requestId = request.messageId;
    context->timeoutMs = m_config.streamTimeoutMs;
    context->cancelFlag = false;
    context->providerName = "InterviewProvider";

    if (!m_sessionRepo.recordUserMessage(request)) {
        InterviewStreamEvent event;
        event.type = InterviewStreamEventType::Error;
        event.error = ApiError{ApiErrorCode::StorageError, "Failed to record user message"};
        session->push(event);
        releaseSlot();
        return;
    }

    std::string assistantText;
    std::string pendingChunk;
    bool doneSeen = false;
    bool errorSeen = false;

    const auto flushPendingChunk = [&]() {
        if (pendingChunk.empty()) {
            return;
        }

        InterviewStreamEvent chunkEvent;
        chunkEvent.type = InterviewStreamEventType::Chunk;
        chunkEvent.content = pendingChunk;
        pendingChunk.clear();
        session->push(chunkEvent);
    };

    auto callback = [&](const InterviewStreamEvent& event) {
        if (session->cancelled()) {
            context->cancelFlag = true;
            return;
        }

        if (event.type == InterviewStreamEventType::Chunk && event.content.has_value()) {
            assistantText += *event.content;
            pendingChunk += *event.content;
            if (!shouldFlushChunkBuffer(pendingChunk, *event.content)) {
                return;
            }
            flushPendingChunk();
            return;
        }

        if (event.type == InterviewStreamEventType::Done) {
            flushPendingChunk();
            doneSeen = true;
            session->push(event);
            return;
        }

        if (event.type == InterviewStreamEventType::Error) {
            flushPendingChunk();
            errorSeen = true;
            session->push(event);
            return;
        }

        session->push(event);
    };

    try {
        // provider 负责把远端流转成本地事件流，manager 这里只接事件并落到 session。
        m_provider.streamFeedback(request, callback, context);

        // 有些 provider 可能没有显式补 Done，这里兜底一次，避免 SSE 一直挂着。
        if (!session->cancelled() && !doneSeen && !errorSeen) {
            InterviewStreamEvent doneEvent;
            doneEvent.type = InterviewStreamEventType::Done;
            session->push(doneEvent);
        }

        if (!assistantText.empty() && !session->cancelled() && !errorSeen) {
            m_sessionRepo.recordAssistantMessage(request, assistantText);
        }
    } catch (const std::exception& e) {
        InterviewStreamEvent errorEvent;
        errorEvent.type = InterviewStreamEventType::Error;
        errorEvent.error = ApiError{ApiErrorCode::ProviderError, e.what()};
        session->push(errorEvent);
    } catch (...) {
        InterviewStreamEvent errorEvent;
        errorEvent.type = InterviewStreamEventType::Error;
        errorEvent.error = ApiError{ApiErrorCode::InternalError, "unknown stream error"};
        session->push(errorEvent);
    }

    // 无论成功还是失败，都要结束 session 并释放并发名额。
    if (!session->cancelled() && session->state() == StreamSession::State::Running) {
        session->setState(errorSeen ? StreamSession::State::Error : StreamSession::State::Done);
    }
    session->close();
    releaseSlot();
}

InterviewStreamManager::Stats InterviewStreamManager::stats() const {
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}
