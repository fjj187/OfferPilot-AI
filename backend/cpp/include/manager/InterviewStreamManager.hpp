#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "manager/StreamSession.hpp"
#include "providers/InterviewProvider.hpp"
#include "repositories/ISessionRepository.hpp"
#include "types/InterviewTypes.hpp"

// InterviewStreamManager 负责管理“会话维度的一条流”。
// 它只做三件事：
// 1. 维护 session/thread 到 StreamSession 的映射；
// 2. 控制全局并发上限，避免同时跑太多流；
// 3. 把 provider 执行投递到后台 worker，让 HTTP 层只负责搬运事件。
//
// 真正的流式状态、队列缓存、取消标记和最终文本拼装，全部放在
// StreamSession 里完成，这样 manager 的职责就足够单一。
class InterviewStreamManager {
public:
    struct Stats {
        size_t accepted = 0;
        size_t completed = 0;
        size_t rejected = 0;
        size_t cancelled = 0;
        size_t active = 0;
        size_t peakActive = 0;
        size_t queued = 0;
        size_t peakQueued = 0;
    };
    // 运行时配置：
    // maxInflightStreams  控制同时运行的流数量，防止把机器打满；
    // workerCount         后台 worker 数量；
    // streamTimeoutMs     provider 级超时；
    // idleTimeoutMs       预留给后续空闲回收逻辑；
    // chunkMergeWindowMs  预留给后续 chunk 合并策略。
    struct Config {
        size_t maxInflightStreams = 128;
        size_t maxQueueSizePerStream = 64;
        size_t workerCount = 4;
        int streamTimeoutMs = 120000;
        int idleTimeoutMs = 30000;
        size_t chunkMergeWindowMs = 50;
    };

    // 提交结果：
    // accepted=true 表示任务已经进入调度队列；
    // accepted=false 表示被拒绝，reason 会说明是 session 不存在、
    // 并发超限，还是当前会话已经在运行。
    struct StartResult {
        bool accepted = false;
        std::string reason;
    };

    InterviewStreamManager(
        InterviewProvider& provider,
        ISessionRepository& sessionRepo,
        Config config);

    ~InterviewStreamManager();

    // 创建一个新的会话缓冲对象。
    // 这里通常在收到一次新的 SSE 请求时调用，用来把请求标识和
    // 对应的 StreamSession 绑定起来。
    std::shared_ptr<StreamSession> createSession(const InterviewStreamRequest& request);

    // 把 provider 执行任务丢进后台 worker。
    // controller 只负责把 session 建起来，真正的模型调用从这里开始。
    StartResult submit(const InterviewStreamRequest& request,
                       const std::shared_ptr<StreamSession>& session);

    // 取消一条正在运行的流。
    // 取消后会立刻唤醒等待中的 SSE 消费者，并让 provider 尽快感知
    // cancelFlag，从而退出远端调用。
    void cancel(const std::string& sessionId, const std::string& threadId);

    // 根据 session/thread 查询内存中的流会话。
    // SSE 输出线程、恢复逻辑、取消逻辑都可能依赖这个查询入口。
    std::shared_ptr<StreamSession> getSession(const std::string& sessionId,
                                              const std::string& threadId);

    // 清理已经结束的会话，避免 session 索引无限增长。
    void clearFinished();
    Stats stats() const;

private:
    struct StreamKey {
        std::string sessionId;
        std::string threadId;

        bool operator==(const StreamKey& other) const {
            return sessionId == other.sessionId && threadId == other.threadId;
        }
    };

    struct StreamKeyHash {
        size_t operator()(const StreamKey& key) const noexcept;
    };

private:
    // worker 生命周期控制。
    void startWorkerLoop();
    void stopWorkers();

    // 执行一次完整的 provider 流式调用。
    // provider 产出的 chunk / done / error 会被写入对应的 StreamSession。
    void executeStream(InterviewStreamRequest request,
                       std::shared_ptr<StreamSession> session);

    // 全局 inflight 计数器，用来实现并发上限。
    bool tryAcquireSlot();
    void releaseSlot();

private:
    InterviewProvider& m_provider;
    ISessionRepository& m_sessionRepo;
    Config m_config;

    mutable std::mutex m_sessionsMutex;
    std::unordered_map<StreamKey, std::shared_ptr<StreamSession>, StreamKeyHash> m_sessions;

    std::mutex m_slotsMutex;
    size_t m_inflightStreams = 0;

    std::vector<std::thread> m_workers;
    std::mutex m_tasksMutex;
    std::condition_variable m_tasksCv;
    std::deque<std::function<void()>> m_tasks;
    bool m_stopping = false;
    mutable std::mutex m_statsMutex;
    Stats m_stats;
};
