#include "services/InterviewService.hpp"

#include <cctype>
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

InterviewService::InterviewService(InterviewProvider &provider, ISessionRepository &repository)
    : m_provider(provider),
      m_sessionRepository(repository) {}

void InterviewService::streamInterview(
    const InterviewStreamRequest& request,
    std::function<void(const InterviewStreamEvent&)> callback,
    std::shared_ptr<ProviderContext> context)
{
    // 这里的执行顺序很重要：先校验，再落用户消息，再调用 provider。
    std::string errorMessage;
    if (!validateRequest(request, errorMessage)) {
        InterviewStreamEvent event;
        event.type = InterviewStreamEventType::Error;
        event.error = ApiError{ApiErrorCode::InvalidRequest, errorMessage};
        callback(event);
        return;
    }

    auto providerContext = context ? std::move(context) : createProviderContext(request);
    if (!recordUserMessage(request)) {
        InterviewStreamEvent event;
        event.type = InterviewStreamEventType::Error;
        event.error = ApiError{ApiErrorCode::StorageError, "Failed to record user message"};
        callback(event);
        return;
    }

    std::string assistantContent;
    std::string pendingChunk;
    bool providerFailed = false;
    bool streamFinished = false;

    const auto flushPendingChunk = [&]() {
        if (pendingChunk.empty()) {
            return;
        }

        InterviewStreamEvent chunkEvent;
        chunkEvent.type = InterviewStreamEventType::Chunk;
        chunkEvent.content = pendingChunk;
        pendingChunk.clear();
        callback(chunkEvent);
    };

    m_provider.streamFeedback(
        request,
        [&](const InterviewStreamEvent& event) {
            // 回调里只做两件事：转发事件，以及拼接 assistant 的完整输出。
            if (event.type == InterviewStreamEventType::Chunk && event.content.has_value()) {
                assistantContent += event.content.value();
                pendingChunk += event.content.value();
                if (!shouldFlushChunkBuffer(pendingChunk, event.content.value())) {
                    return;
                }
                flushPendingChunk();
                return;
            }

            if (event.type == InterviewStreamEventType::Error) {
                providerFailed = true;
                flushPendingChunk();
                callback(event);
                return;
            }

            if (event.type == InterviewStreamEventType::Done) {
                streamFinished = true;
                flushPendingChunk();
                callback(event);
                return;
            }

            callback(event);
        },
        providerContext);

    // 有些 provider 可能不会显式发送 Done，这里补一次终止事件，避免前端挂住。
    if (!providerFailed && !streamFinished) {
        InterviewStreamEvent event;
        event.type = InterviewStreamEventType::Done;
        callback(event);
    }

    // provider 正常结束后，再把完整 assistant 消息写回仓储。
    if (!providerFailed && streamFinished) {
        if (!recordAssistantMessage(request, assistantContent)) {
            InterviewStreamEvent event;
            event.type = InterviewStreamEventType::Error;
            event.error = ApiError{ApiErrorCode::StorageError, "Failed to record assistant message"};
            callback(event);
        }
    }
}

bool InterviewService::validateRequest(const InterviewStreamRequest& request, std::string& errorMessage) {
    // 流式面试接口字段较多，这里逐个校验，方便前端快速定位问题。
    const auto requireField = [&](const std::string& value, const char* name) -> bool {
        if (!value.empty()) {
            return true;
        }
        errorMessage = std::string("Missing required field: ") + name;
        return false;
    };

    if (!requireField(request.sessionId, "sessionId")) return false;
    if (!requireField(request.messageId, "messageId")) return false;
    if (!requireField(request.threadId, "threadId")) return false;
    if (!requireField(request.topic, "topic")) return false;
    if (!requireField(request.topicLabel, "topicLabel")) return false;
    if (!requireField(request.prompt, "prompt")) return false;
    if (!requireField(request.questionTitle, "questionTitle")) return false;
    if (!requireField(request.questionPrompt, "questionPrompt")) return false;
    if (!requireField(request.answer, "answer")) return false;
    return true;
}

std::shared_ptr<ProviderContext> InterviewService::createProviderContext(const InterviewStreamRequest& request) {
    // 给 provider 构造默认上下文，支持超时和取消控制。
    auto context = std::make_shared<ProviderContext>();
    context->requestId = request.sessionId + ":" + request.messageId;
    context->timeoutMs = 30000;
    context->cancelFlag = false;
    context->providerName = "InterviewProvider";
    return context;
}

bool InterviewService::recordUserMessage(const InterviewStreamRequest& request) {
    return m_sessionRepository.recordUserMessage(request);
}

bool InterviewService::recordAssistantMessage(const InterviewStreamRequest& request, const std::string& assistantContent) {
    return m_sessionRepository.recordAssistantMessage(request, assistantContent);
}

std::vector<InterviewSessionSummary> InterviewService::listSessions() {
    return m_sessionRepository.listSessions();
}

std::optional<InterviewSessionDetail> InterviewService::getSession(const std::string& sessionId, const std::string& threadId) {
    return m_sessionRepository.getSession(sessionId, threadId);
}

void InterviewService::clearHistory() {
    m_sessionRepository.clearAll();
}
