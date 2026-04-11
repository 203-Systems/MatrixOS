/**
 * MystrixSIL JSON-RPC 2.0 dispatcher
 *
 * Transport: in-page function call.
 *   window.matrixosRpc.call(method, params) → Promise<result>
 *   window.matrixosRpc.subscribe(topic, callback) → unsubscribe()
 *
 * All 22 methods from MystrixSILJsonRpcSpec.md are implemented here.
 * The UI panels continue to use Svelte stores directly (low-ceremony);
 * this layer adds a clean, documented, scriptable API surface on top.
 */

import { get } from 'svelte/store'
import {
  moduleReady, runtimeStatus, versionLabel, buildIdentity,
  doReboot, sendGridKey, sendFnKey, getUptimeMs,
} from './wasm.js'
import { logMessages } from './logs.js'
import { midiEvents, midiPorts, portLabel, portHex, sendMidiNote, sendMidiCC, sendMidiProgramChange } from './midi.js'
import { hidEvents, sendRawHid } from './hid.js'
import { serialEvents, sendSerialText, sendSerialHex } from './serial.js'
import { nvsEntries, refreshNvs, writeNvsEntry, deleteNvsEntry } from './storage.js'

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

const PROTOCOL_VERSION = '0.1'
const SESSION_ID = 'sess-' + Math.random().toString(36).slice(2, 10)

const ERR = {
  INVALID_PARAMS: { code: 4001, message: 'Invalid params' },
  UNKNOWN_TARGET:  { code: 4002, message: 'Unknown target' },
  UNSUPPORTED:     { code: 4003, message: 'Unsupported capability' },
  TIMEOUT:         { code: 4004, message: 'Timeout' },
  INTERNAL:        { code: 5001, message: 'Internal error' },
}

// ---------------------------------------------------------------------------
// Global emulator error collector
// ---------------------------------------------------------------------------

const _emulatorErrors = []

function collectError(severity, source, message) {
  _emulatorErrors.push({
    timestamp: new Date().toISOString(),
    severity,
    source,
    message: String(message),
  })
  if (_emulatorErrors.length > 200) _emulatorErrors.shift()
}

function installErrorCollectors() {
  const prevError = window.onerror
  window.onerror = (msg, src, line, col, err) => {
    collectError('error', 'window.onerror', `${msg} (${src}:${line}:${col})`)
    if (typeof prevError === 'function') return prevError(msg, src, line, col, err)
  }

  const prevUnhandled = window.onunhandledrejection
  window.onunhandledrejection = (evt) => {
    collectError('error', 'unhandledrejection', String(evt.reason))
    if (typeof prevUnhandled === 'function') prevUnhandled(evt)
  }
}

// ---------------------------------------------------------------------------
// NVS hash — FNV-1a 32-bit, matching MatrixOS key hashing
// ---------------------------------------------------------------------------

export function computeNvsHash(text) {
  let hash = 0x811c9dc5
  const bytes = new TextEncoder().encode(text)
  for (const byte of bytes) {
    hash ^= byte
    hash = (Math.imul(hash, 0x01000193)) >>> 0
  }
  return hash
}

export function nvsHashHex(text) {
  return '0x' + computeNvsHash(text).toString(16).padStart(8, '0').toUpperCase()
}

// ---------------------------------------------------------------------------
// Subscription system (for midi/hid/serial push notifications)
// ---------------------------------------------------------------------------

const _subs = { midi: [], hid: [], serial: [] }

function subscribe(topic, callback) {
  if (!_subs[topic]) return () => {}
  _subs[topic].push(callback)
  return () => {
    _subs[topic] = _subs[topic].filter(cb => cb !== callback)
  }
}

let _prevMidiLen = 0
let _prevHidLen = 0
let _prevSerialLen = 0

function initSubscriptionBridge() {
  midiEvents.subscribe(evts => {
    if (evts.length > _prevMidiLen) {
      const newEvts = evts.slice(_prevMidiLen)
      newEvts.forEach(evt => {
        _subs.midi.forEach(cb => cb({
          timestamp: new Date().toISOString(),
          direction: evt.direction === 'TX' ? 'out' : 'in',
          sourcePort: evt.srcPortLabel || evt.srcPort,
          targetPort: evt.dstPortLabel || evt.dstPort,
          message: {
            kind: evt.msgType,
            channel: evt.channel,
            data: [evt.data0, evt.data1, evt.data2],
            summary: evt.summary,
          },
        }))
      })
    }
    _prevMidiLen = evts.length
  })

  hidEvents.subscribe(evts => {
    if (evts.length > _prevHidLen) {
      const newEvts = evts.slice(_prevHidLen)
      newEvts.forEach(evt => {
        _subs.hid.forEach(cb => cb({
          timestamp: new Date().toISOString(),
          category: evt.category,
          direction: evt.direction === 'TX' ? 'out' : 'in',
          payload: evt.rawPayload,
        }))
      })
    }
    _prevHidLen = evts.length
  })

  serialEvents.subscribe(evts => {
    if (evts.length > _prevSerialLen) {
      const newEvts = evts.slice(_prevSerialLen)
      newEvts.forEach(evt => {
        _subs.serial.forEach(cb => cb({
          timestamp: new Date().toISOString(),
          direction: evt.direction === 'TX' ? 'out' : 'in',
          payload: evt.text,
          encoding: 'utf8',
        }))
      })
    }
    _prevSerialLen = evts.length
  })
}

// ---------------------------------------------------------------------------
// Input helpers
// ---------------------------------------------------------------------------

const INPUT_GRID_SIZE = 8

function parseInputId(id) {
  if (id === 'function') return { kind: 'functionKey' }
  const m = /^grid:(\d+),(\d+)$/.exec(id)
  if (m) {
    const x = parseInt(m[1]), y = parseInt(m[2])
    if (x >= 0 && x < INPUT_GRID_SIZE && y >= 0 && y < INPUT_GRID_SIZE) {
      return { kind: 'gridKey', x, y }
    }
  }
  return null
}

function execInputAction(target, action) {
  if (target.kind === 'functionKey') {
    sendFnKey(action === 'Press')
  } else if (target.kind === 'gridKey') {
    sendGridKey(target.x, target.y, action === 'Press')
  }
}

// ---------------------------------------------------------------------------
// LED / framebuffer helpers
// ---------------------------------------------------------------------------

function getFramebufferData() {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_GetFrameBuffer || !mod.HEAPU8) return null
  const ptr = mod._MatrixOS_Wasm_GetFrameBuffer()
  const byteLen = mod._MatrixOS_Wasm_GetFrameBufferByteLength?.() ?? 0
  if (!ptr || !byteLen) return null
  return mod.HEAPU8.subarray(ptr, ptr + byteLen)
}

function ledAtIndex(data, idx) {
  const base = idx * 4
  return {
    r: data[base],
    g: data[base + 1],
    b: data[base + 2],
    w: data[base + 3],
  }
}

function ledToHex({ r, g, b, w }) {
  return (
    r.toString(16).padStart(2, '0') +
    g.toString(16).padStart(2, '0') +
    b.toString(16).padStart(2, '0') +
    w.toString(16).padStart(2, '0')
  ).toUpperCase()
}

// ---------------------------------------------------------------------------
// RPC method handlers
// ---------------------------------------------------------------------------

const handlers = {

  // ---- Session ----

  'session.status': () => ({
    protocolVersion: PROTOCOL_VERSION,
    sessionId: SESSION_ID,
    connected: true,
    runtimeReady: get(moduleReady),
    build: get(buildIdentity) || get(versionLabel) || 'Matrix OS',
  }),

  'session.ping': () => ({ ok: true }),

  'session.reset': () => {
    doReboot()
    return { ok: true }
  },

  // ---- Runtime ----

  'runtime.getState': () => {
    const ready = get(moduleReady)
    return {
      status: get(runtimeStatus),
      uptimeMs: getUptimeMs(),
      usbConnected: false, // runtime USB state not yet exposed by WASM
      activeApp: ready ? { name: 'Running', id: 'runtime' } : null,
      warningCount: 0,
      errorCount: _emulatorErrors.filter(e => e.severity === 'error').length,
    }
  },

  'runtime.getAppState': () => ({
    activeApp: get(moduleReady) ? { name: 'Running', id: 'runtime' } : null,
  }),

  // ---- Input ----

  'input.list': () => {
    const inputs = [
      { id: 'function', label: 'Function Key', kind: 'functionKey' },
    ]
    for (let y = 0; y < INPUT_GRID_SIZE; y++) {
      for (let x = 0; x < INPUT_GRID_SIZE; x++) {
        inputs.push({ id: `grid:${x},${y}`, label: `Grid ${x},${y}`, kind: 'gridKey', x, y })
      }
    }
    return { inputs }
  },

  'input.execute': (params) => {
    const events = params?.events
    if (!Array.isArray(events) || events.length === 0) {
      return { __error: ERR.INVALID_PARAMS }
    }

    let accepted = 0
    for (const evt of events) {
      const target = parseInputId(evt.input)
      if (!target) continue
      const action = evt.action // 'Press' | 'Release' | 'Hold'
      if (action !== 'Press' && action !== 'Release') continue // Hold not injectable

      const atMs = typeof evt.atMs === 'number' ? evt.atMs : 0
      if (atMs <= 0) {
        execInputAction(target, action)
      } else {
        setTimeout(() => execInputAction(target, action), atMs)
      }
      accepted++
    }

    return { ok: true, accepted }
  },

  'input.get': () => {
    const mod = window.Module
    const activeInputs = []

    if (mod?._MatrixOS_Wasm_GetFnState?.() !== 0) {
      activeInputs.push({ input: 'function', label: 'Function Key' })
    }

    if (mod?._MatrixOS_Wasm_GetKeypadState && mod?._MatrixOS_Wasm_GetKeypadStateLength) {
      const heap = mod.HEAPU8
      const len = mod._MatrixOS_Wasm_GetKeypadStateLength()
      const ptr = mod._MatrixOS_Wasm_GetKeypadState()
      if (ptr && len && heap) {
        const state = heap.subarray(ptr, ptr + len)
        for (let i = 0; i < state.length; i++) {
          if (state[i]) {
            const x = i % INPUT_GRID_SIZE
            const y = Math.floor(i / INPUT_GRID_SIZE)
            activeInputs.push({ input: `grid:${x},${y}`, label: `Grid ${x},${y}` })
          }
        }
      }
    }

    return { activeInputs }
  },

  // ---- LED ----

  'led.getFrame': () => {
    const data = getFramebufferData()
    if (!data) return { __error: ERR.UNSUPPORTED }

    const totalLeds = Math.floor(data.length / 4)
    const gridCount = Math.min(64, totalLeds)
    const ugCount = Math.max(0, totalLeds - 64)

    const grid = []
    for (let i = 0; i < gridCount; i++) grid.push(ledToHex(ledAtIndex(data, i)))

    const underglow = []
    for (let i = 64; i < 64 + ugCount; i++) underglow.push(ledToHex(ledAtIndex(data, i)))

    return {
      timestamp: new Date().toISOString(),
      format: 'rgbw-hex',
      grid,
      underglow,
    }
  },

  'led.get': (params) => {
    const id = params?.id
    if (!id) return { __error: ERR.INVALID_PARAMS }

    const data = getFramebufferData()
    if (!data) return { __error: ERR.UNSUPPORTED }

    let idx
    const gridM = /^grid:(\d+),(\d+)$/.exec(id)
    const ugM = /^underglow:(\d+)$/.exec(id)
    if (gridM) {
      const x = parseInt(gridM[1]), y = parseInt(gridM[2])
      if (x < 0 || x >= INPUT_GRID_SIZE || y < 0 || y >= INPUT_GRID_SIZE) {
        return { __error: ERR.UNKNOWN_TARGET }
      }
      idx = y * INPUT_GRID_SIZE + x
    } else if (ugM) {
      idx = 64 + parseInt(ugM[1])
    } else {
      return { __error: ERR.UNKNOWN_TARGET }
    }

    if (idx * 4 + 3 >= data.length) return { __error: ERR.UNKNOWN_TARGET }
    return { id, color: ledToHex(ledAtIndex(data, idx)) }
  },

  // ---- Logs ----

  'log.get': (params) => {
    let entries = get(logMessages).map(m => ({
      timestamp: m.timestamp,
      level: m.level === 'warn' ? 'warning' : m.level,
      tag: m.tag || 'Unknown',
      text: m.text,
    }))

    if (params?.level) {
      const levels = Array.isArray(params.level) ? params.level : [params.level]
      entries = entries.filter(e => levels.includes(e.level))
    }
    if (typeof params?.last === 'number') {
      entries = entries.slice(-params.last)
    }

    return { entries }
  },

  'emulator.getErrors': (params) => {
    let entries = [..._emulatorErrors]

    if (params?.severity) {
      const sev = Array.isArray(params.severity) ? params.severity : [params.severity]
      entries = entries.filter(e => sev.includes(e.severity))
    }
    if (typeof params?.last === 'number') {
      entries = entries.slice(-params.last)
    }

    return { entries }
  },

  // ---- MIDI ----

  'midi.listPorts': () => {
    const ports = get(midiPorts).map(p => ({
      id: '0x' + p.id.toString(16).padStart(4, '0').toUpperCase(),
      name: p.name,
    }))
    return { ports }
  },

  'midi.send': (params) => {
    const msg = params?.message
    if (!msg) return { __error: ERR.INVALID_PARAMS }

    const rawTarget = params.targetPort ?? params.destinationPort
    const targetPort = typeof rawTarget === 'string'
      ? parseInt(rawTarget.replace(/^0x/i, ''), 16)
      : (typeof rawTarget === 'number' ? rawTarget : 0x0100)

    const ch = typeof msg.channel === 'number' ? (msg.channel - 1) & 0x0F : 0

    const kind = msg.kind?.toLowerCase()
    if (kind === 'noteon' || kind === 'note_on') {
      sendMidiNote(ch, msg.note ?? 60, msg.velocity ?? 100, targetPort)
    } else if (kind === 'noteoff' || kind === 'note_off') {
      sendMidiNote(ch, msg.note ?? 60, 0, targetPort)
    } else if (kind === 'cc' || kind === 'controlchange' || kind === 'control_change') {
      sendMidiCC(ch, msg.controller ?? 0, msg.value ?? 0, targetPort)
    } else if (kind === 'programchange' || kind === 'program_change') {
      sendMidiProgramChange(ch, msg.program ?? 0, targetPort)
    } else {
      return { __error: ERR.INVALID_PARAMS }
    }

    return { ok: true }
  },

  'midi.subscribe': (params, notifyFn) => {
    if (typeof notifyFn !== 'function') return { __error: ERR.INVALID_PARAMS }
    const unsub = subscribe('midi', notifyFn)
    return { ok: true, __cleanup: unsub }
  },

  // ---- HID ----

  'hid.send': (params) => {
    const kind = params?.kind?.toLowerCase()
    const payload = params?.payload
    if (!payload) return { __error: ERR.INVALID_PARAMS }

    if (kind === 'rawhid' || kind === 'raw_hid' || !kind) {
      const encoding = params.encoding ?? 'hex'
      let bytes
      if (encoding === 'hex') {
        bytes = payload.trim().split(/\s+/).map(h => parseInt(h, 16)).filter(n => !isNaN(n))
      } else {
        bytes = [...new TextEncoder().encode(payload)]
      }
      const ok = sendRawHid(new Uint8Array(bytes))
      return ok ? { ok: true } : { __error: ERR.UNSUPPORTED }
    }

    return { __error: ERR.INVALID_PARAMS }
  },

  'hid.subscribe': (params, notifyFn) => {
    if (typeof notifyFn !== 'function') return { __error: ERR.INVALID_PARAMS }
    const unsub = subscribe('hid', notifyFn)
    return { ok: true, __cleanup: unsub }
  },

  // ---- Serial ----

  'serial.send': (params) => {
    const payload = params?.payload
    if (typeof payload !== 'string') return { __error: ERR.INVALID_PARAMS }

    const encoding = params.encoding ?? 'utf8'
    let ok
    if (encoding === 'hex') {
      ok = sendSerialHex(payload)
    } else {
      ok = sendSerialText(payload)
    }
    return ok ? { ok: true } : { __error: ERR.UNSUPPORTED }
  },

  'serial.subscribe': (params, notifyFn) => {
    if (typeof notifyFn !== 'function') return { __error: ERR.INVALID_PARAMS }
    const unsub = subscribe('serial', notifyFn)
    return { ok: true, __cleanup: unsub }
  },

  // ---- Storage / NVS ----

  'storage.nvs.find': (params) => {
    const hashFilter = params?.hash?.toLowerCase()
    let entries = get(nvsEntries)

    if (hashFilter) {
      entries = entries.filter(e => e.hashHex.toLowerCase().includes(hashFilter))
    }

    return {
      entries: entries.map(e => ({
        hash: e.hashHex,
        size: e.size,
        valuePreview: e.preview,
      })),
    }
  },

  'storage.nvs.get': (params) => {
    const hash = parseNvsHash(params?.hash)
    if (hash === null) return { __error: ERR.INVALID_PARAMS }

    const entry = get(nvsEntries).find(e => e.hash === hash)
    if (!entry) return { __error: ERR.UNKNOWN_TARGET }

    return {
      hash: entry.hashHex,
      size: entry.size,
      valuePreview: entry.preview,
      rawBytes: entry.rawBytes,
    }
  },

  'storage.nvs.set': (params) => {
    const hash = parseNvsHash(params?.hash)
    if (hash === null) return { __error: ERR.INVALID_PARAMS }

    const value = params?.value
    if (typeof value !== 'string') return { __error: ERR.INVALID_PARAMS }

    const encoding = params.encoding ?? 'hex'
    let bytes
    if (encoding === 'hex') {
      bytes = value.trim().split(/\s+/).map(h => parseInt(h, 16)).filter(n => !isNaN(n))
    } else {
      bytes = [...new TextEncoder().encode(value)]
    }

    const ok = writeNvsEntry(hash, new Uint8Array(bytes))
    return ok ? { ok: true } : { __error: ERR.INTERNAL }
  },

  'storage.nvs.computeHash': (params) => {
    const text = params?.text
    if (typeof text !== 'string') return { __error: ERR.INVALID_PARAMS }
    const hash = computeNvsHash(text)
    return {
      text,
      hash: '0x' + hash.toString(16).padStart(8, '0').toUpperCase(),
    }
  },
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function parseNvsHash(raw) {
  if (raw === null || raw === undefined) return null
  if (typeof raw === 'number') return raw >>> 0
  if (typeof raw === 'string') {
    const n = parseInt(raw.replace(/^0x/i, ''), 16)
    return isNaN(n) ? null : n >>> 0
  }
  return null
}

// ---------------------------------------------------------------------------
// Dispatcher
// ---------------------------------------------------------------------------

let _reqCounter = 0

async function dispatch(method, params = {}, notifyFn = null) {
  const id = 'rpc-' + (++_reqCounter)
  const handler = handlers[method]

  if (!handler) {
    return {
      jsonrpc: '2.0',
      id,
      error: { code: -32601, message: 'Method not found' },
    }
  }

  try {
    const result = handler(params, notifyFn)
    if (result?.__error) {
      return { jsonrpc: '2.0', id, error: result.__error }
    }
    // Strip internal keys
    const { __cleanup, __error, ...clean } = result ?? {}
    return { jsonrpc: '2.0', id, result: clean }
  } catch (err) {
    collectError('error', `rpc.${method}`, err?.message ?? String(err))
    return {
      jsonrpc: '2.0',
      id,
      error: { ...ERR.INTERNAL, detail: String(err?.message ?? err) },
    }
  }
}

// ---------------------------------------------------------------------------
// Public API object exposed at window.matrixosRpc
// ---------------------------------------------------------------------------

export const rpcApi = {
  call: dispatch,
  subscribe,
  computeNvsHash,
  nvsHashHex,
  get sessionId() { return SESSION_ID },
  get protocolVersion() { return PROTOCOL_VERSION },
}

// ---------------------------------------------------------------------------
// Init — call once from App.svelte after initWasm()
// ---------------------------------------------------------------------------

export function initRpc() {
  installErrorCollectors()
  initSubscriptionBridge()
  window.matrixosRpc = rpcApi
  console.info('[MystrixSIL] JSON-RPC ready. Use window.matrixosRpc.call(method, params) to interact.')
}
