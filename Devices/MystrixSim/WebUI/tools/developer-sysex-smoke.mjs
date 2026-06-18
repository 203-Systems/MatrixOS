#!/usr/bin/env node
import { WebSocket } from 'ws'

const DEFAULT_WS = 'ws://localhost:4002'
const TIMEOUT_MS = 10_000

const DEVELOPER_APP_ID = '0xBCC0FAEF'
const DEVELOPER_PING = 'F0 00 02 03 4D 58 00 F7'
const DEVELOPER_SET_INPUT = 'F0 00 02 03 4D 58 01 01 F7'
const DEVELOPER_LED_CLEAR = 'F0 00 02 03 4D 58 21 01 F7'
const DEVELOPER_SYSEX_PREFIX = [0xF0, 0x00, 0x02, 0x03, 0x4D, 0x58]
const REPLY_COMMAND_BASE = 0x40

let nextId = 1
const midiEvents = []

function parseArgs() {
  const args = process.argv.slice(2)
  let wsUrl = DEFAULT_WS

  for (let i = 0; i < args.length; i++) {
    const arg = args[i]
    if (arg === '--ws') {
      wsUrl = args[++i] || DEFAULT_WS
    } else if (arg === '--help' || arg === '-h') {
      console.log('Usage: node tools/developer-sysex-smoke.mjs [--ws ws://localhost:4002]')
      process.exit(0)
    } else {
      throw new Error(`Unknown argument: ${arg}`)
    }
  }

  return { wsUrl }
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms))
}

function connect(wsUrl) {
  return new Promise((resolve, reject) => {
    const socket = new WebSocket(wsUrl)
    const timer = setTimeout(() => reject(new Error('Connect timeout')), TIMEOUT_MS)
    socket.on('open', () => {
      clearTimeout(timer)
      resolve(socket)
    })
    socket.on('error', error => {
      clearTimeout(timer)
      reject(error)
    })
  })
}

function rpcCall(socket, method, params = {}, timeoutMs = TIMEOUT_MS) {
  return new Promise((resolve, reject) => {
    const id = `developer-sysex-${nextId++}`
    const requestParams = {
      ...(params ?? {}),
      rpcTimeoutMs: Math.max(1000, timeoutMs - 500),
    }
    const timer = setTimeout(() => {
      socket.off('message', onMessage)
      reject(new Error(`Timeout waiting for ${method}`))
    }, timeoutMs)

    function onMessage(data) {
      let payload
      try { payload = JSON.parse(data.toString()) } catch { return }
      if (payload?.type === 'connection_count' || payload?.id !== id) return

      clearTimeout(timer)
      socket.off('message', onMessage)
      if (payload.error) {
        reject(new Error(`${method}: ${JSON.stringify(payload.error)}`))
        return
      }
      resolve(payload.result)
    }

    socket.on('message', onMessage)
    socket.send(JSON.stringify({ jsonrpc: '2.0', id, method, params: requestParams }))
  })
}

function watchMidiNotifications(socket) {
  socket.on('message', data => {
    let payload
    try { payload = JSON.parse(data.toString()) } catch { return }
    if (payload?.method === 'midi.event') {
      midiEvents.push(payload.params)
    }
  })
}

function parseHexBytes(payload) {
  return String(payload).trim().split(/\s+/).filter(Boolean).map(token => parseInt(token, 16))
}

function eventBytes(event) {
  if (!Array.isArray(event?.message?.data)) return []
  return event.message.data.map(byte => Number(byte) & 0xFF)
}

function containsSubsequence(bytes, pattern) {
  if (pattern.length === 0) return true

  for (let i = 0; i <= bytes.length - pattern.length; i++) {
    let matched = true
    for (let j = 0; j < pattern.length; j++) {
      if (bytes[i + j] !== pattern[j]) {
        matched = false
        break
      }
    }
    if (matched) return true
  }

  return false
}

function findDeveloperAppId(appState) {
  const apps = Array.isArray(appState?.applications) ? appState.applications : []
  const app = apps.find(item => item?.name === 'Developer' && (item?.author === '203 Systems' || item?.author === '203Null' || !item?.author))
    ?? apps.find(item => item?.name === 'Developer')
  return app?.id ?? app?.appId ?? app?.app_id ?? DEVELOPER_APP_ID
}

async function waitForDeveloperAck(command, startIndex = 0, timeoutMs = 2000) {
  const pattern = [...DEVELOPER_SYSEX_PREFIX, REPLY_COMMAND_BASE | command, 0x00]
  const deadline = Date.now() + timeoutMs
  while (Date.now() < deadline) {
    const bytes = midiEvents.slice(startIndex).flatMap(eventBytes)
    if (containsSubsequence(bytes, pattern)) return true
    await sleep(50)
  }
  return false
}

async function run() {
  const { wsUrl } = parseArgs()
  const socket = await connect(wsUrl)
  watchMidiNotifications(socket)

  const bridge = await rpcCall(socket, 'bridge.status')
  if (!bridge?.runtimeConnected) {
    throw new Error('Runtime bridge is not connected. Open the MystrixSim WebUI tab first.')
  }
  console.log('[developer-sysex-smoke] bridge', JSON.stringify(bridge))

  await rpcCall(socket, 'midi.subscribe', {})

  const state = await rpcCall(socket, 'runtime.getState')
  if (state?.activeApp?.name !== 'Developer') {
    const appState = await rpcCall(socket, 'application.list')
    const developerAppId = findDeveloperAppId(appState)
    console.log('[developer-sysex-smoke] launching Developer')
    await rpcCall(socket, 'application.launch', { appId: developerAppId })
    await sleep(500)
  }

  for (const payload of [DEVELOPER_PING, DEVELOPER_SET_INPUT, DEVELOPER_LED_CLEAR]) {
    const command = parseHexBytes(payload)[6]
    const before = midiEvents.length
    await rpcCall(socket, 'midi.sendSysEx', { payload })
    console.log('[developer-sysex-smoke] tx', payload)

    if (payload !== DEVELOPER_LED_CLEAR) {
      const ok = await waitForDeveloperAck(command, before)
      if (!ok) {
        const recent = midiEvents.slice(before)
        throw new Error(`Did not observe Developer SysEx reply for command 0x${command.toString(16).padStart(2, '0').toUpperCase()}. Recent MIDI events: ${JSON.stringify(recent)}`)
      }
    }
  }

  console.log('[developer-sysex-smoke] observed', midiEvents.length, 'MIDI events')
  socket.close()
}

run().catch(error => {
  console.error(`[developer-sysex-smoke] failed: ${error.message}`)
  process.exit(1)
})
