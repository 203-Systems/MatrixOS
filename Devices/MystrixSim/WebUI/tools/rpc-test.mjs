#!/usr/bin/env node
/**
 * rpc-test.mjs — legacy filename retained; now a WebSocket presence tester.
 *
 * External JSON-RPC over WS is disabled. This tool now opens a plain WS
 * connection to the local dev-mode server so you can verify the connection
 * count shown in the WebUI.
 *
 * Usage:
 *   node tools/rpc-test.mjs [--ws ws://localhost:4002] [--hold-ms 10000]
 */

import { WebSocket } from 'ws'

const DEFAULT_WS = 'ws://localhost:4002'
const DEFAULT_HOLD_MS = 10_000
const TIMEOUT_MS = 10_000

function parseArgs() {
  const idx = process.argv.indexOf('--ws')
  const holdIdx = process.argv.indexOf('--hold-ms')
  return {
    wsUrl: idx !== -1 ? process.argv[idx + 1] : DEFAULT_WS,
    holdMs: holdIdx !== -1 ? Number(process.argv[holdIdx + 1]) || DEFAULT_HOLD_MS : DEFAULT_HOLD_MS,
  }
}

function sleep(ms) {
  return new Promise(r => setTimeout(r, ms))
}

async function run() {
  const { wsUrl, holdMs } = parseArgs()
  console.log(`[rpc-test] connecting to ${wsUrl}`)

  const ws = new WebSocket(wsUrl)

  await new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error('Connect timeout')), TIMEOUT_MS)
    ws.on('open', () => {
      clearTimeout(timer)
      resolve()
    })
    ws.on('error', (err) => {
      clearTimeout(timer)
      reject(err)
    })
  }).catch((err) => {
    console.error(`[rpc-test] connection failed: ${err.message}`)
    console.error('  → Is npm run dev running?')
    process.exit(1)
  })

  ws.on('message', (data) => {
    let msg
    try { msg = JSON.parse(data.toString()) } catch { return }
    if (msg.type === 'connection_count') {
      const count = Number(msg.externalConnectionCount ?? msg.connectionCount) || 0
      console.log(`[rpc-test] external clients: ${count}`)
    }
  })

  console.log(`[rpc-test] connected; holding for ${holdMs} ms`)
  await sleep(holdMs)
  ws.close()
  console.log('[rpc-test] done')
}

run()
