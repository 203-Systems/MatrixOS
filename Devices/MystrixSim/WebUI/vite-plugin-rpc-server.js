/**
 * vite-plugin-rpc-server.js
 *
 * Vite plugin that starts a local WebSocket JSON-RPC server on RPC_WS_PORT
 * when the dev server is running (apply: 'serve').
 *
 * Architecture:
 *   - External clients connect and send JSON-RPC 2.0 requests
 *   - The browser agent (src/stores/wsbridge.js) connects and registers
 *     with { role: 'agent' } to handle requests inside the live WASM session
 *   - Server proxies client requests → agent, and routes responses back
 *
 * Port must match RPC_WS_PORT in src/stores/wsbridge.js.
 */

import { WebSocketServer } from 'ws'

export const RPC_WS_PORT = 4002

export function rpcServerPlugin() {
  return {
    name: 'matrixos-rpc-server',
    apply: 'serve',

    configureServer(server) {
      const wss = new WebSocketServer({ port: RPC_WS_PORT })

      /** @type {import('ws').WebSocket|null} */
      let agentSocket = null

      // proxyId → client WebSocket
      const pending = new Map()

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
        ws.on('message', (data) => {
          let msg
          try { msg = JSON.parse(data.toString()) } catch { return }

          // ---- Browser agent registration ----
          if (msg.role === 'agent') {
            agentSocket = ws
            ws._isAgent = true
            ws.send(JSON.stringify({ type: 'agent_ack', port: RPC_WS_PORT }))
            return
          }

          // ---- Response coming back from the browser agent ----
          if (ws._isAgent) {
            const { __proxyId, response } = msg
            const clientWs = pending.get(__proxyId)
            pending.delete(__proxyId)
            if (clientWs && clientWs.readyState === 1 /* OPEN */) {
              clientWs.send(JSON.stringify(response))
            }
            return
          }

          // ---- External client request ----
          if (!agentSocket || agentSocket.readyState !== 1 /* OPEN */) {
            ws.send(JSON.stringify({
              jsonrpc: '2.0',
              id: msg.id ?? null,
              error: { code: -32603, message: 'Browser agent not connected' },
            }))
            return
          }

          const proxyId = `p-${Date.now()}-${Math.random().toString(36).slice(2)}`
          pending.set(proxyId, ws)
          agentSocket.send(JSON.stringify({ ...msg, __proxyId: proxyId }))
        })

        ws.on('close', () => {
          if (ws._isAgent && agentSocket === ws) agentSocket = null
        })

        ws.on('error', () => {})
      })

      server.httpServer?.once('close', () => wss.close())
    },
  }
}
