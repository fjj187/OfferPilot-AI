#!/usr/bin/env node
// Reproducible SSE benchmark for the C++ backend.
// Usage: node bench/sse-benchmark.mjs --base http://127.0.0.1:3030 --concurrency 32 --duration 30 --pid 1234
import { performance } from 'node:perf_hooks'
import { execFile } from 'node:child_process'
import { promisify } from 'node:util'

const exec = promisify(execFile)
const arg = (name, fallback) => { const i = process.argv.indexOf(name); return i >= 0 ? process.argv[i + 1] : fallback }
const base = arg('--base', 'http://127.0.0.1:3030').replace(/\/$/, '')
const concurrency = Number(arg('--concurrency', '16'))
const duration = Number(arg('--duration', '30')) * 1000
const pid = arg('--pid', '')
const cancelConcurrency = Number(arg('--cancel-concurrency', String(Math.max(4, Math.min(concurrency, 32)))))
const cancelAfterMs = Number(arg('--cancel-after-ms', '100'))
const requestTimeoutMs = Number(arg('--request-timeout-ms', '15000'))
if (pid && !/^\d+$/.test(pid)) throw new Error('--pid must be numeric')
if (!Number.isInteger(concurrency) || concurrency < 1) throw new Error('--concurrency must be a positive integer')
if (!Number.isInteger(cancelConcurrency) || cancelConcurrency < 1) throw new Error('--cancel-concurrency must be a positive integer')
if (!Number.isFinite(cancelAfterMs) || cancelAfterMs < 0) throw new Error('--cancel-after-ms must be >= 0')
if (!Number.isFinite(requestTimeoutMs) || requestTimeoutMs < 100) throw new Error('--request-timeout-ms must be >= 100')
const firstChunkLatencies = [], totalLatencies = [], clientAbortLatencies = [], statuses = {}
const errors = {}, timeouts = {}
let attempts = 0

const payload = (i) => ({ sessionId: `bench-${Date.now()}-${i}`, threadId: `thread-${i}`, messageId: `message-${Date.now()}-${i}`, topic: 'backend', topicLabel: 'C++ backend', prompt: 'benchmark', questionTitle: 'benchmark', questionPrompt: 'Explain a thread pool', answer: 'A bounded worker pool processes tasks.' })
const recordStatus = (s) => { statuses[s] = (statuses[s] || 0) + 1 }

async function one(i, { cancelAfterMs: abortAfterMs = 0 } = {}) {
  attempts += 1
  const started = performance.now(); const controller = new AbortController()
  let abortAt = 0
  let abortTimer, timeoutTimer
  if (abortAfterMs > 0) abortTimer = setTimeout(() => { abortAt = performance.now(); controller.abort() }, abortAfterMs)
  const timeoutController = new AbortController()
  timeoutTimer = setTimeout(() => timeoutController.abort(), requestTimeoutMs)
  try {
    const signal = AbortSignal.any([controller.signal, timeoutController.signal])
    const res = await fetch(`${base}/api/interview/stream`, { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify(payload(i)), signal })
    recordStatus(res.status)
    if (!res.ok) return { ok: false, status: res.status }
    const reader = res.body.getReader(); let first = true
    while (true) { const { done } = await reader.read(); if (done) break; if (first) { firstChunkLatencies.push(performance.now() - started); first = false } }
    totalLatencies.push(performance.now() - started)
    return { ok: true, status: res.status }
  } catch (e) {
    if (abortAt) clientAbortLatencies.push(performance.now() - abortAt)
    else if (timeoutController.signal.aborted) timeouts[e?.name || 'timeout'] = (timeouts[e?.name || 'timeout'] || 0) + 1
    else errors[e?.name || 'request_error'] = (errors[e?.name || 'request_error'] || 0) + 1
    return { ok: false, error: e?.name || 'request_error' }
  } finally {
    if (abortTimer) clearTimeout(abortTimer)
    if (timeoutTimer) clearTimeout(timeoutTimer)
  }
}
async function sampleProcess(samples) {
  if (!pid) return
  try { const { stdout } = await exec('powershell.exe', ['-NoProfile', '-Command', `(Get-Process -Id ${pid}).WorkingSet64, (Get-Process -Id ${pid}).Threads.Count`]); const v = stdout.trim().split(/\s+/).map(Number); if (v.length >= 2) samples.push({ rssBytes: v[0], threads: v[1] }) } catch {}
}
async function readMetrics() {
  try { return await (await fetch(`${base}/api/metrics`)).json() } catch { return null }
}

const metricsBefore = await readMetrics()
const samples = []; const end = Date.now() + duration
let lastProcessSampleAt = 0
let requestId = 0
await Promise.all(Array.from({ length: concurrency }, async (_, worker) => {
  while (Date.now() < end) {
    const result = await one(worker * 1000000 + requestId++)
    if (result?.error) {
      // Avoid turning a dead server into a tight client-side retry loop.
      await new Promise(resolve => setTimeout(resolve, 50))
    }
    if (Date.now() - lastProcessSampleAt >= 1000) {
      lastProcessSampleAt = Date.now()
      await sampleProcess(samples)
    }
  }
}))
// Cancellation probe: abort active requests after 100 ms.
await Promise.all(Array.from({ length: cancelConcurrency }, (_, i) => one(10000 + i, { cancelAfterMs })))
const percentile = (xs, p) => { if (!xs.length) return null; const a = [...xs].sort((x, y) => x - y); return a[Math.min(a.length - 1, Math.ceil(p * a.length) - 1)] }
const summary = (xs) => ({ avg: xs.length ? xs.reduce((a, b) => a + b, 0) / xs.length : null, p99: percentile(xs, .99), max: percentile(xs, 1), samples: xs.length })
const metricsAfter = await readMetrics()
console.log(JSON.stringify({
  base, concurrency, durationMs: duration, cancelConcurrency, cancelAfterMs, requestTimeoutMs,
  attempts, requests: totalLatencies.length, firstChunkLatencyMs: summary(firstChunkLatencies),
  totalLatencyMs: summary(totalLatencies),
  clientAbortToFetchErrorMs: summary(clientAbortLatencies),
  statuses, errors, timeouts,
  process: { rssMinBytes: samples.length ? Math.min(...samples.map(x => x.rssBytes)) : null, rssMaxBytes: samples.length ? Math.max(...samples.map(x => x.rssBytes)) : null, threadsMin: samples.length ? Math.min(...samples.map(x => x.threads)) : null, threadsMax: samples.length ? Math.max(...samples.map(x => x.threads)) : null },
  serverMetrics: { before: metricsBefore, after: metricsAfter }
}, null, 2))
