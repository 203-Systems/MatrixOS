/**
 * wsbridge.js — Browser-side WebSocket agent bridge.
 *
 * In local dev mode (IS_NODE_BACKED), this module connects to the WS RPC
 * server started by vite-plugin-rpc-server.js, registers as the browser
 * agent, and dispatches incoming requests through the canonical rpcApi.
 *
 * Port must match RPC_WS_PORT in vite-plugin-rpc-server.js.
 */

import { writable } from 'svelte/store'
import { IS_NODE_BACKED, rpcApi } from './rpc.js'

// Must match vite-plugin-rpc-server.js
export const RPC_WS_PORT = 4002
export const RPC_WS_URL = IS_NODE_BACKED ? `ws://localhost:${RPC_WS_PORT}` : ''

/**
 * Reactive status for ConnectionPage.svelte.
 * Values: 'unavailable' | 'connecting' | 'connected' | 'disconnected'
 */
export const wsBridgeStatus = writable(IS_NODE_BACKED ? 'connecting' : 'unavailable')

let _ws = null

export function initWsBridge() {
  if (!IS_NODE_BACKED) return
  // Guard against HMR re-registration
  if (window.__matrixosWsBridgeInit) return
  window.__matrixosWsBridgeInit = true
  _connect()
}

function _connect() {
  wsBridgeStatus.set('connecting')

  let ws
  try {
    ws = new WebSocket(RPC_WS_URL)
  } catch {
    wsBridgeStatus.set('disconnected')
    setTimeout(_connect, 3000)
    return
  }

  _ws = ws

  ws.onopen = () => {
    ws.send(JSON.stringify({ role: 'agent' }))
  }

  ws.onmessage = async (event) => {
    let msg
    try { msg = JSON.parse(event.data) } catch { return }

    // Server acknowledges agent registration
    if (msg.type === 'agent_ack') {
      wsBridgeStatus.set('connected')
      return
    }

    // Proxied JSON-RPC request from an external client
    const { __proxyId, method, params, id } = msg
    if (!method || __proxyId === undefined) return

    const response = await rpcApi.call(method, params ?? {})
    response.id = id ?? null
    ws.send(JSON.stringify({ __proxyId, response }))
  }

  ws.onclose = () => {
    if (_ws === ws) {
      _ws = null
      wsBridgeStatus.set('disconnected')
      setTimeout(_connect, 3000)
    }
  }

  ws.onerror = () => {}
}
