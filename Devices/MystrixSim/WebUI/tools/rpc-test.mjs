#!/usr/bin/env node
/**
 * rpc-test.mjs — MystrixSim WebSocket JSON-RPC smoke tester.
 *
 * Usage:
 *   node tools/rpc-test.mjs
 *   node tools/rpc-test.mjs --ws ws://localhost:4002
 *   node tools/rpc-test.mjs --method storage.fs.list --params "{\"path\":\"/\"}"
 */

import { WebSocket } from 'ws'

const DEFAULT_WS = 'ws://localhost:4002'
const TIMEOUT_MS = 10_000

function parseArgs() {
  const wsIndex = process.argv.indexOf('--ws')
  const methodIndex = process.argv.indexOf('--method')
  const paramsIndex = process.argv.indexOf('--params')

  let params = {}
  if (paramsIndex !== -1) {
    try {
      params = JSON.parse(process.argv[paramsIndex + 1] || '{}')
    } catch (error) {
      console.error(`[rpc-test] invalid --params JSON: ${error.message}`)
      process.exit(1)
    }
  }

  return {
    wsUrl: wsIndex !== -1 ? process.argv[wsIndex + 1] : DEFAULT_WS,
    method: methodIndex !== -1 ? process.argv[methodIndex + 1] : '',
    params,
  }
}

function connect(wsUrl) {
  return new Promise((resolve, reject) => {
    const socket = new WebSocket(wsUrl)
    const timer = setTimeout(() => reject(new Error('Connect timeout')), TIMEOUT_MS)

    socket.on('open', () => {
      clearTimeout(timer)
      resolve(socket)
    })

    socket.on('error', (error) => {
      clearTimeout(timer)
      reject(error)
    })
  })
}

function rpcCall(socket, method, params = {}) {
  return new Promise((resolve, reject) => {
    const id = `rpc-test-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`
    const timer = setTimeout(() => {
      socket.off('message', onMessage)
      reject(new Error(`Timeout waiting for ${method}`))
    }, TIMEOUT_MS)

    function onMessage(data) {
      let payload = null
      try {
        payload = JSON.parse(data.toString())
      } catch {
        return
      }

      if (payload?.type === 'connection_count') {
        return
      }

      if (payload?.id !== id) {
        return
      }

      clearTimeout(timer)
      socket.off('message', onMessage)
      resolve(payload)
    }

    socket.on('message', onMessage)
    socket.send(JSON.stringify({
      jsonrpc: '2.0',
      id,
      method,
      params,
    }))
  })
}

async function runSmokeSuite(socket) {
  const calls = [
    ['session.status', {}],
    ['runtime.getState', {}],
    ['storage.fs.status', {}],
    ['storage.fs.list', { path: '/' }],
  ]

  for (const [method, params] of calls) {
    const response = await rpcCall(socket, method, params)
    console.log(`\n[rpc-test] ${method}`)
    console.log(JSON.stringify(response, null, 2))
  }
}

async function run() {
  const { wsUrl, method, params } = parseArgs()
  console.log(`[rpc-test] connecting to ${wsUrl}`)

  const socket = await connect(wsUrl).catch((error) => {
    console.error(`[rpc-test] connection failed: ${error.message}`)
    console.error('  → Is npm run dev running, and is the MystrixSim page open?')
    process.exit(1)
  })

  console.log('[rpc-test] connected')

  try {
    if (method) {
      const response = await rpcCall(socket, method, params)
      console.log(JSON.stringify(response, null, 2))
    } else {
      await runSmokeSuite(socket)
    }
  } finally {
    socket.close()
  }
}

run().catch((error) => {
  console.error(`[rpc-test] failed: ${error.message}`)
  process.exit(1)
})
