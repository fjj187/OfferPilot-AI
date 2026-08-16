# C++ SSE performance benchmark

Start the C++ backend with `USE_MOCK_INTERVIEW_PROVIDER=1`, then run:

```powershell
node bench/sse-benchmark.mjs --base http://127.0.0.1:3030 --concurrency 32 --duration 30 --pid <backend-pid> > bench-result.json
```

The JSON report contains average/P99/max time-to-first chunk and full-stream latency, cancellation latency, HTTP status counts, process Working Set/thread min/max, and `/api/metrics` counters. `serverMetrics.streams.peakActive` is the maximum simultaneous stream count; `rejected` is the behavior when the inflight limit is full. `mysql.hitRate` is idle-connection reuse (`idleHits / acquireRequests`); run with the real MySQL repository to include database traffic.
