#!/usr/bin/env node
/**
 * test-hot-restart.mjs
 *
 * Unit-tests the hot-restart path via the WebSocket JSON-RPC proxy.
 *
 * Usage (with dev server running + browser open at localhost:5173):
 *   node test-hot-restart.mjs
 *
 * The test:
 *  0. Calls session.fullReload to ensure the browser has latest code,
 *     then reconnects and waits for the runtime to be Live.
 *  1. Calls session.reset — hot-restart (no page reload)
 *  2. Waits for runtime to return to Live (polls every 2s, up to 15s)
 *  3. Calls session.reset again to verify repeated resets are safe
 *  4. Prints PASS / FAIL with reasons
 */

import { WebSocket } from 'ws'

const WS_URL = 'ws://localhost:4002'
const CONNECT_TIMEOUT_MS = 5_000
const RPC_TIMEOUT_MS = 20_000    // How long to wait for a single RPC response
const RUNTIME_POLL_MS = 15_000   // How long to poll for runtime Live state

let id = 1
const nextId = () => id++

function connect() {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(WS_URL)
    ws.on('open', () => resolve(ws))
    ws.on('error', reject)
    setTimeout(() => reject(new Error(`Connect timeout to ${WS_URL}`)), CONNECT_TIMEOUT_MS)
  })
}

function call(ws, method, params = {}) {
  return new Promise((resolve, reject) => {
    const reqId = nextId()
    const timer = setTimeout(() => reject(new Error(`Timeout waiting for ${method}`)), RPC_TIMEOUT_MS)

    const handler = (data) => {
      let msg
      try { msg = JSON.parse(data.toString()) } catch { return }
      if (msg.id !== reqId) return
      ws.off('message', handler)
      clearTimeout(timer)
      if (msg.error) reject(new Error(`RPC error: ${JSON.stringify(msg.error)}`))
      else resolve(msg.result)
    }
    ws.on('message', handler)
    ws.send(JSON.stringify({ jsonrpc: '2.0', id: reqId, method, params }))
    console.log(`  → ${method}`)
  })
}

async function sleep(ms) {
  return new Promise(r => setTimeout(r, ms))
}

/** Poll runtime.getState until status === 'Live', or until timeoutMs elapses. */
async function waitForLive(ws, timeoutMs = RUNTIME_POLL_MS) {
  const start = Date.now()
  while (Date.now() - start < timeoutMs) {
    await sleep(2000)
    try {
      const state = await call(ws, 'runtime.getState')
      if (state?.status === 'Live') return state
      console.log(`    … still ${state?.status} at ${Math.round((Date.now() - start) / 1000)}s`)
    } catch { /* agent disconnected or reloading */ }
  }
  return null
}

async function run() {
  let ws
  let passed = 0
  let failed = 0

  function ok(msg) { console.log(`  ✓ ${msg}`); passed++ }
  function fail(msg) { console.error(`  ✗ ${msg}`); failed++ }

  async function dumpDiagnostics(label) {
    console.log(`\n  [Diagnostics: ${label}]`)
    try {
      const errs = await call(ws, 'emulator.getErrors', { last: 10 })
      if (errs.entries.length === 0) {
        console.log('    No emulator errors')
      } else {
        errs.entries.forEach(e => console.log(`    ERROR [${e.source}]: ${e.message}`))
      }
    } catch (e) { console.log(`    (emulator.getErrors failed: ${e.message})`) }
    try {
      const logs = await call(ws, 'log.get', { last: 5 })
      if (logs.entries.length === 0) {
        console.log('    No MatrixOS logs')
      } else {
        logs.entries.forEach(e => console.log(`    LOG [${e.level}] ${e.tag}: ${e.text}`))
      }
    } catch (e) { console.log(`    (log.get failed: ${e.message})`) }
  }

  try {
    console.log(`\nConnecting to ${WS_URL}…`)
    ws = await connect()
    console.log('Connected.\n')

    // ---- Step 0: Full page reload to ensure latest code is loaded ----
    console.log('[Step 0] session.fullReload — ensuring browser has latest code')
    try {
      const res = await call(ws, 'session.fullReload')
      if (res?.ok) {
        console.log('  Browser reloading… waiting for agent to reconnect')
        ws.close()
        ws = null
        await sleep(3000)

        // Reconnect (the page reload disconnects and reconnects the agent)
        for (let i = 0; i < 10; i++) {
          try {
            ws = await connect()
            console.log('  Agent reconnected.')
            break
          } catch {
            if (i < 9) { await sleep(2000); console.log(`  … waiting for agent (attempt ${i + 2})`) }
            else throw new Error('Agent did not reconnect after page reload')
          }
        }

        // Wait for runtime to come Live after the fresh page load
        console.log('  Waiting for runtime to come Live after reload…')
        const liveState = await waitForLive(ws, 20_000)
        if (liveState) {
          ok(`Runtime Live after reload (uptime ${liveState.uptimeMs}ms)`)
        } else {
          fail('Runtime not Live after page reload — check browser and dev server')
          return
        }
      } else {
        // session.fullReload not supported by old code — skip reload step
        console.log('  session.fullReload not supported — skipping code refresh')
        console.log('  WARNING: tests may fail if browser has old code')
        const state = await call(ws, 'runtime.getState').catch(() => null)
        if (state?.status === 'Live') ok(`Baseline runtime is Live (uptime ${state.uptimeMs}ms)`)
        else fail(`Baseline runtime not Live: ${JSON.stringify(state)}`)
      }
    } catch (e) {
      // If method not found, old browser — skip reload
      if (e.message.includes('Method not found')) {
        console.log('  session.fullReload not available — testing with existing browser code')
        const state = await call(ws, 'runtime.getState').catch(() => null)
        if (state?.status === 'Live') ok(`Baseline runtime is Live (uptime ${state.uptimeMs}ms)`)
        else fail(`Baseline: ${JSON.stringify(state)}`)
      } else {
        fail(`session.fullReload failed: ${e.message}`)
      }
    }

    // ---- Test 1: session.reset — hot-restart (no page reload) ----
    console.log('\n[Test 1] session.reset — hot-restart, runtime should return Live')
    try {
      const res = await call(ws, 'session.reset')
      ok(`session.reset responded: ${JSON.stringify(res)}`)
    } catch (e) {
      fail(`session.reset failed: ${e.message}`)
    }

    console.log(`\n  Polling for runtime Live (up to ${RUNTIME_POLL_MS / 1000}s)…`)
    const state1 = await waitForLive(ws)
    if (state1) {
      ok(`Runtime Live after reset (uptime ${state1.uptimeMs}ms)`)
    } else {
      fail('Runtime not Live after session.reset — hot-restart failed')
      await dumpDiagnostics('after first reset')
    }

    // ---- Test 2: second session.reset — repeated restarts must be safe ----
    console.log('\n[Test 2] Second session.reset — repeated restarts must be safe')
    try {
      const res = await call(ws, 'session.reset')
      ok(`second reset responded: ${JSON.stringify(res)}`)
    } catch (e) {
      fail(`second session.reset failed: ${e.message}`)
    }

    console.log(`\n  Polling for runtime Live (up to ${RUNTIME_POLL_MS / 1000}s)…`)
    const state2 = await waitForLive(ws)
    if (state2) {
      ok(`Runtime Live after second reset (uptime ${state2.uptimeMs}ms)`)
    } else {
      fail('Runtime not Live after second session.reset')
      await dumpDiagnostics('after second reset')
    }

  } catch (e) {
    console.error(`\nFatal error: ${e.message}`)
    failed++
  } finally {
    if (ws) ws.close()
    console.log(`\n${'─'.repeat(50)}`)
    console.log(`Results: ${passed} passed, ${failed} failed`)
    console.log('─'.repeat(50))
    process.exit(failed > 0 ? 1 : 0)
  }
}

run()
