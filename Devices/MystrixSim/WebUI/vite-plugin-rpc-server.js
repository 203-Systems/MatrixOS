/**
 * vite-plugin-rpc-server.js
 *
 * Starts a local WebSocket server on RPC_WS_PORT when the Vite dev server is
 * running (apply: 'serve').
 *
 * This server is status-only. The browser page does not connect back to it,
 * and in-page RPC remains a direct JS API call via window.matrixosRpc.
 *
 * External clients may connect so the UI can count them, but JSON-RPC over WS
 * is intentionally disabled.
 */

import { WebSocketServer } from 'ws'

export const RPC_WS_PORT = 4002
export const RPC_DISCOVERY_PATH = '/__matrixos/rpc-status'

export function rpcServerPlugin() {
  return {
    name: 'matrixos-rpc-server',
    apply: 'serve',

    configureServer(server) {
      const wss = new WebSocketServer({ port: RPC_WS_PORT })

      function getExternalConnectionCount() {
        let count = 0
        for (const client of wss.clients) {
          if (client.readyState === 1 /* OPEN */) count += 1
        }
        return count
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

      wss.on('connection', (ws) => {
        broadcastConnectionCount()

        ws.on('message', (data) => {
          let msg
          try { msg = JSON.parse(data.toString()) } catch { return }
          ws.send(JSON.stringify({
            jsonrpc: '2.0',
            id: msg.id ?? null,
            error: {
              code: -32601,
              message: 'External WebSocket RPC is disabled. Use window.matrixosRpc in the browser tab.',
            },
          }))
        })

        ws.on('close', () => {
          broadcastConnectionCount()
        })

        ws.on('error', () => {})
      })

      server.httpServer?.once('close', () => wss.close())
    },
  }
}
