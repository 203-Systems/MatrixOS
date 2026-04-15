#!/usr/bin/env node
/**
 * rpc-test.mjs — MystrixSim JSON-RPC WebSocket smoke tester
 *
 * Connects to the local dev-mode WebSocket server (ws://localhost:4002)
 * and runs a sequence of JSON-RPC calls against the live WASM session.
 *
 * Usage:
 *   node tools/rpc-test.mjs [--ws ws://localhost:4002]
 *
 * Requirements:
 *   - npm run dev must be running (starts ws://localhost:4002)
 *   - The browser tab must be open so the WS bridge is active
 *
 * Sequence:
 *   1. session.reset
 *   2. input.execute — Function Key tap
 *   3. wait 2 s
 *   4. input.execute — grid:0,3 tap
 *   5. runtime.getAppState
 *   6. led.getFrame
 */

import { WebSocket } from 'ws'

const DEFAULT_WS = 'ws://localhost:4002'
const TIMEOUT_MS = 10_000

function parseArgs() {
  const idx = process.argv.indexOf('--ws')
  return idx !== -1 ? process.argv[idx + 1] : DEFAULT_WS
}

class RpcClient {
  constructor(url) {
    this.url = url
    this._id = 0
    this._pending = new Map()
    this._ws = null
  }

  connect() {
    return new Promise((resolve, reject) => {
      this._ws = new WebSocket(this.url)
      const timer = setTimeout(() => reject(new Error('Connect timeout')), TIMEOUT_MS)
      this._ws.on('open', () => { clearTimeout(timer); resolve() })
      this._ws.on('error', (err) => { clearTimeout(timer); reject(err) })
      this._ws.on('message', (data) => {
        let msg
        try { msg = JSON.parse(data.toString()) } catch { return }
        const { id, result, error } = msg
        const cb = this._pending.get(id)
        if (!cb) return
        this._pending.delete(id)
        if (error) cb.reject(new Error(`${error.code}: ${error.message}`))
        else cb.resolve(result)
      })
    })
  }

  call(method, params = {}) {
    return new Promise((resolve, reject) => {
      const id = ++this._id
      const timer = setTimeout(() => {
        this._pending.delete(id)
        reject(new Error(`Timeout waiting for ${method}`))
      }, TIMEOUT_MS)

      this._pending.set(id, {
        resolve: (v) => { clearTimeout(timer); resolve(v) },
        reject:  (e) => { clearTimeout(timer); reject(e) },
      })

      this._ws.send(JSON.stringify({ jsonrpc: '2.0', id, method, params }))
    })
  }

  close() {
    this._ws?.close()
  }
}

function sleep(ms) {
  return new Promise(r => setTimeout(r, ms))
}

async function run() {
  const wsUrl = parseArgs()
  console.log(`[rpc-test] connecting to ${wsUrl}`)

  const client = new RpcClient(wsUrl)

  try {
    await client.connect()
    console.log('[rpc-test] connected\n')
  } catch (err) {
    console.error(`[rpc-test] connection failed: ${err.message}`)
    console.error('  → Is npm run dev running and is the browser tab open?')
    process.exit(1)
  }

  try {
    // 1. session.reset
    console.log('→ session.reset')
    const reset = await client.call('session.reset')
    console.log('  ', JSON.stringify(reset))

    await sleep(400)

    // 2. Function Key tap
    console.log('→ input.execute (Function Key tap)')
    const fnTap = await client.call('input.execute', {
      events: [
        { input: 'function', action: 'Press' },
        { input: 'function', action: 'Release', atMs: 120 },
      ],
    })
    console.log('  ', JSON.stringify(fnTap))

    // 3. wait 2 s
    console.log('→ waiting 2 s…')
    await sleep(2000)

    // 4. grid:0,3 tap
    console.log('→ input.execute (grid:0,3 tap)')
    const gridTap = await client.call('input.execute', {
      events: [
        { input: 'grid:0,3', action: 'Press' },
        { input: 'grid:0,3', action: 'Release', atMs: 120 },
      ],
    })
    console.log('  ', JSON.stringify(gridTap))

    await sleep(200)

    // 5. runtime.getAppState
    console.log('→ runtime.getAppState')
    const appState = await client.call('runtime.getAppState')
    console.log(JSON.stringify(appState, null, 2))

    // 6. led.getFrame
    console.log('→ led.getFrame')
    const frame = await client.call('led.getFrame')
    console.log(JSON.stringify({
      timestamp: frame.timestamp,
      format: frame.format,
      gridLedCount: frame.grid?.length ?? 0,
      underglowLedCount: frame.underglow?.length ?? 0,
      firstGridRow: frame.grid?.slice(0, 8) ?? [],
    }, null, 2))

    console.log('\n[rpc-test] done ✓')
  } catch (err) {
    console.error(`[rpc-test] error: ${err.message}`)
    process.exit(1)
  } finally {
    client.close()
  }
}

run()
