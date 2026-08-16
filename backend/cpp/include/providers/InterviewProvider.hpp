#pragma once
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include "types/InterviewTypes.hpp"  // InterviewStreamRequest, InterviewStreamEvent


// Provider 调用上下文（用于取消、超时等）
struct ProviderContext {
    std::string requestId;
    int timeoutMs = 30000;          // 默认 30 秒
    std::atomic<bool> cancelFlag{false}; // 可由外部置 true 来取消
    std::string providerName;
};

class InterviewProvider {
public:
    virtual ~InterviewProvider() = default;

    // 流式生成面试反馈
    // @param request      完整的流式请求对象
    // @param callback     每产生一个事件就回调一次（chunk / done / error）
    // @param context      可选上下文，主要用于取消控制
    virtual void streamFeedback(
        const InterviewStreamRequest& request,
        std::function<void(const InterviewStreamEvent&)> callback,
        std::shared_ptr<ProviderContext> context = nullptr) = 0;
};
