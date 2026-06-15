/**
 * vite-plugin-rpc-server.js
 *
 * Starts a local WebSocket JSON-RPC bridge server on RPC_WS_PORT when the Vite
 * dev server is running (apply: 'serve').
 *
 * The browser page connects back as the active runtime bridge. External
 * clients can then issue JSON-RPC requests over WebSocket and have them
 * executed by the live page/runtime.
 */

import { WebSocketServer } from 'ws'
import { GITHUB_RELEASE_ASSET_PROXY_PATH, handleGitHubReleaseAssetProxy } from './server/github-release-asset-proxy.js'

export const RPC_WS_PORT = Number(process.env.MATRIXOS_RPC_PORT ?? 4002)
export const RPC_DISCOVERY_PATH = '/__matrixos/rpc-status'
const DEFAULT_PENDING_TIMEOUT_MS = 45_000

export function rpcServerPlugin() {
    function installReleaseAssetProxy(server) {
      server.middlewares.use((req, res, next) => {
        const url = req.url ? req.url.split('?')[0] : ''
        if (url !== GITHUB_RELEASE_ASSET_PROXY_PATH) {
          next()
          return
        }

        void handleGitHubReleaseAssetProxy(req, res, req.url)
      })
    }

  return {
    name: 'matrixos-rpc-server',

    configureServer(server) {
      installReleaseAssetProxy(server)
      const wss = new WebSocketServer({ port: RPC_WS_PORT })
      const pendingRequests = new Map()
      const subscriptionRequests = new Map()
      let runtimeClient = null

      function nextRequestId() {
        return `bridge-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`
      }

      function getExternalConnectionCount() {
        let count = 0
        for (const client of wss.clients) {
          if (client.readyState === 1 /* OPEN */ && client._matrixRole !== 'runtime') count += 1
        }
        return count
      }

      function clearPendingRequest(requestId) {
        const pending = pendingRequests.get(requestId)
        if (!pending) return null
        if (pending.timeoutHandle) clearTimeout(pending.timeoutHandle)
        pendingRequests.delete(requestId)
        return pending
      }

      function describePendingRequests() {
        const now = Date.now()
        return Array.from(pendingRequests.entries()).map(([requestId, pending]) => ({
          requestId,
          method: pending.method,
          ageMs: now - pending.startedAt,
          clientRequestId: pending.clientRequestId,
          isSubscription: pending.isSubscription,
        }))
      }

      function broadcastConnectionCount() {
        const externalConnectionCount = getExternalConnectionCount()
        const payload = JSON.stringify({
          type: 'connection_count',
          externalConnectionCount,
          connectionCount: externalConnectionCount,
        })

        for (const client of wss.clients) {
          if (client.readyState === 1 /* OPEN */) {
            client.send(payload)
          }
        }
      }

      server.middlewares.use((req, res, next) => {
        const url = req.url ? req.url.split('?')[0] : ''
        if (url !== RPC_DISCOVERY_PATH) {
          next()
          return
        }

        res.statusCode = 200
        res.setHeader('Content-Type', 'application/json; charset=utf-8')
        res.setHeader('Cache-Control', 'no-store')
        res.end(JSON.stringify({
          ok: true,
          wsUrl: `ws://localhost:${RPC_WS_PORT}`,
          externalConnectionCount: getExternalConnectionCount(),
          runtimeConnected: runtimeClient?.readyState === 1,
        }))
      })

      wss.on('listening', () => {
        console.info(`\n[MatrixOS RPC] WebSocket server ready → ws://localhost:${RPC_WS_PORT}\n`)
      })

      wss.on('error', (err) => {
        if (err.code === 'EADDRINUSE') {
          console.warn(`\n[MatrixOS RPC] Port ${RPC_WS_PORT} already in use — WebSocket RPC server not started.\n`)
        } else {
          console.error(`\n[MatrixOS RPC] WebSocket server error: ${err.message}\n`)
        }
      })

      wss.on('connection', (ws, request) => {
        const requestUrl = new URL(request.url ?? '/', `ws://localhost:${RPC_WS_PORT}`)
        const role = requestUrl.searchParams.get('role') === 'runtime' || ws.protocol === 'matrixos-runtime'
          ? 'runtime'
          : 'external'
        ws._matrixRole = role

        if (role === 'runtime') {
          if (runtimeClient && runtimeClient !== ws && runtimeClient.readyState === 1) {
            runtimeClient.close(1000, 'Superseded by a newer MatrixOS runtime bridge')
          }
          runtimeClient = ws
        }

        broadcastConnectionCount()

        ws.on('message', (data) => {
          let msg
          try { msg = JSON.parse(data.toString()) } catch { return }

          if (ws._matrixRole === 'runtime') {
            if (msg?.type === 'rpc_notification' && msg.requestId) {
              const subscription = subscriptionRequests.get(msg.requestId)
              if (!subscription || subscription.client.readyState !== 1) return

              subscription.client.send(JSON.stringify({
                jsonrpc: '2.0',
                method: msg.method,
                params: msg.params ?? {},
              }))
              return
            }

            if (msg?.type !== 'rpc_response' || !msg.requestId) return

            const pending = clearPendingRequest(msg.requestId)
            if (!pending) return

            if (pending.client.readyState !== 1) return

            if (pending.isSubscription && !msg.response?.error) {
              subscriptionRequests.set(msg.requestId, pending)
            }

            pending.client.send(JSON.stringify({
              jsonrpc: '2.0',
              id: pending.clientRequestId,
              ...(msg.response?.error ? { error: msg.response.error } : { result: msg.response?.result ?? null }),
            }))
            return
          }

          if (!msg || msg.jsonrpc !== '2.0' || typeof msg.method !== 'string') {
            ws.send(JSON.stringify({
              jsonrpc: '2.0',
              id: msg?.id ?? null,
              error: {
                code: -32600,
                message: 'Invalid request',
              },
            }))
            return
          }

          if (msg.method === 'bridge.status') {
            ws.send(JSON.stringify({
              jsonrpc: '2.0',
              id: msg.id ?? null,
              result: {
                ok: true,
                runtimeConnected: runtimeClient?.readyState === 1,
                externalConnectionCount: getExternalConnectionCount(),
                pendingRequestCount: pendingRequests.size,
                pendingRequests: describePendingRequests(),
                subscriptionCount: subscriptionRequests.size,
              },
            }))
            return
          }

          if (!runtimeClient || runtimeClient.readyState !== 1) {
            ws.send(JSON.stringify({
              jsonrpc: '2.0',
              id: msg.id ?? null,
              error: {
                code: 4003,
                message: 'Runtime bridge unavailable. Open the MystrixSim WebUI tab first.',
              },
            }))
            return
          }

          const requestId = nextRequestId()
          const startedAt = Date.now()
          const timeoutMs = Number.isFinite(msg.params?.rpcTimeoutMs)
            ? Math.max(1000, Number(msg.params.rpcTimeoutMs))
            : DEFAULT_PENDING_TIMEOUT_MS
          const timeoutHandle = setTimeout(() => {
            const pending = clearPendingRequest(requestId)
            if (!pending || pending.client.readyState !== 1) return
            pending.client.send(JSON.stringify({
              jsonrpc: '2.0',
              id: pending.clientRequestId,
              error: {
                code: 4004,
                message: 'Runtime bridge request timed out.',
                detail: `${pending.method} did not respond within ${timeoutMs}ms.`,
              },
            }))
          }, timeoutMs)

          pendingRequests.set(requestId, {
            client: ws,
            clientRequestId: msg.id ?? null,
            method: msg.method,
            startedAt,
            timeoutHandle,
            isSubscription: msg.method.endsWith('.subscribe'),
          })

          runtimeClient.send(JSON.stringify({
            type: 'rpc_request',
            requestId,
            method: msg.method,
            params: msg.params ?? {},
          }))
        })

        ws.on('close', () => {
          if (runtimeClient === ws) {
            runtimeClient = null
            for (const [requestId, pending] of pendingRequests.entries()) {
              clearPendingRequest(requestId)
              if (pending.client.readyState === 1) {
                pending.client.send(JSON.stringify({
                  jsonrpc: '2.0',
                  id: pending.clientRequestId,
                  error: {
                    code: 4003,
                    message: 'Runtime bridge disconnected before the request completed.',
                  },
                }))
              }
              pendingRequests.delete(requestId)
            }
            subscriptionRequests.clear()
          } else {
            for (const [requestId, pending] of pendingRequests.entries()) {
              if (pending.client === ws) {
                clearPendingRequest(requestId)
              }
            }
            for (const [requestId, subscription] of subscriptionRequests.entries()) {
              if (subscription.client === ws) {
                subscriptionRequests.delete(requestId)
              }
            }
          }
          broadcastConnectionCount()
        })

        ws.on('error', () => {})
      })

      server.httpServer?.once('close', () => wss.close())
    },

		configurePreviewServer(server) {
			installReleaseAssetProxy(server)
		},
  }
}
