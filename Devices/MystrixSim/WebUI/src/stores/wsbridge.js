/**
 * wsbridge.js — WebSocket server status store.
 *
 * In-page RPC uses window.matrixosRpc directly. The UI does not connect to its
 * own WebSocket server. We only poll a same-origin discovery endpoint exposed
 * by vite-plugin-rpc-server.js to learn whether the local WS server exists and
 * how many external clients are currently connected.
 */

import { writable } from 'svelte/store'
import { IS_NODE_BACKED } from './rpc.js'

// Must match vite-plugin-rpc-server.js
export const RPC_WS_PORT = 4002
export const RPC_WS_URL = IS_NODE_BACKED ? `ws://localhost:${RPC_WS_PORT}` : ''
const RPC_DISCOVERY_PATH = '/__matrixos/rpc-status'
const STATUS_POLL_MS = 2000

/**
 * Reactive status for ConnectionPage.svelte and TopBar.svelte.
 * Values: 'unavailable' | 'available' | 'connected'
 *
 *   unavailable — static build only
 *   available   — node-backed mode is active, but there are no remote WS clients
 *   connected   — one or more remote WS clients are attached
 */
export const wsBridgeStatus = writable(IS_NODE_BACKED ? 'available' : 'unavailable')
export const wsBridgeConnectionCount = writable(0)

let _statusRefreshPromise = null

export function initWsBridge() {
  if (!IS_NODE_BACKED) return
  if (window.__matrixosWsBridgeDispose) return window.__matrixosWsBridgeDispose

  window.__matrixosWsBridgeStatusInit = true
  void refreshWsBridgeStatus()
  const pollHandle = window.setInterval(() => {
    void refreshWsBridgeStatus()
  }, STATUS_POLL_MS)

  const dispose = () => {
    window.clearInterval(pollHandle)
    if (window.__matrixosWsBridgeDispose === dispose) {
      window.__matrixosWsBridgeDispose = null
      window.__matrixosWsBridgeStatusInit = false
    }
  }

  window.__matrixosWsBridgeDispose = dispose
  return dispose
}

export async function refreshWsBridgeStatus() {
  if (!IS_NODE_BACKED) {
    wsBridgeConnectionCount.set(0)
    wsBridgeStatus.set('unavailable')
    return
  }

  if (_statusRefreshPromise) return _statusRefreshPromise

  _statusRefreshPromise = _readServerStatus()
    .then(({ available, externalConnectionCount }) => {
      const connectionCount = Number(externalConnectionCount) || 0
      wsBridgeConnectionCount.set(connectionCount)
      wsBridgeStatus.set(!available ? 'unavailable' : connectionCount > 0 ? 'connected' : 'available')
    })
    .catch(() => {
      wsBridgeConnectionCount.set(0)
      wsBridgeStatus.set('unavailable')
    })
    .finally(() => {
      _statusRefreshPromise = null
    })

  return _statusRefreshPromise
}

async function _readServerStatus() {
  try {
    const response = await fetch(RPC_DISCOVERY_PATH, {
      method: 'GET',
      cache: 'no-store',
      headers: { accept: 'application/json' },
    })
    if (!response.ok) {
      return { available: false, externalConnectionCount: 0 }
    }

    const payload = await response.json()
    return {
      available: true,
      externalConnectionCount: Number(payload.externalConnectionCount) || 0,
    }
  } catch {
    return { available: false, externalConnectionCount: 0 }
  }
}
