/**
 * MystrixSim JSON-RPC 2.0 dispatcher
 *
 * Transport: in-page function call.
 *   window.matrixosRpc.call(method, params) → Promise<result>
 *   window.matrixosRpc.subscribe(topic, callback) → unsubscribe()
 *
 * Implements all 22 handles from MystrixSimJsonRpcSpec.md.
 *
 * Deployment modes:
 *   IS_NODE_BACKED = true  → dev server (npm run dev) or a build with
 *                            VITE_MATRIXOS_RPC_EXTERNAL=true.
 *                            window.matrixosRpc is exposed in this tab.
 *                            No external WebSocket transport is implemented.
 *   IS_NODE_BACKED = false → static browser-only build (npm run build).
 *                            window.matrixosRpc is NOT exposed.
 *                            JSON-RPC shows as Unavailable in the UI.
 */

import { get } from 'svelte/store'
import { logMessages } from './logs.js'
import { midiEvents, midiPorts } from './midi.js'
import { hidEvents } from './hid.js'
import { serialEvents } from './serial.js'
import { nvsEntries, writeNvsEntry, computeNvsHash, nvsHashHex } from './storage.js'
import { listInputs, executeInputEvents, getActiveInputs } from '../handles/input.js'
import { getLedFrame, getLed } from '../handles/led.js'
import { sendMidiNote, sendMidiCC, sendMidiProgramChange } from '../handles/midi.js'
import { sendRawHid } from '../handles/hid.js'
import { sendSerialText, sendSerialHex } from '../handles/serial.js'
import {
  getSessionStatus,
  pingSession,
  resetSession,
  getRuntimeState,
  getRuntimeAppState,
} from '../handles/session.js'

// ---------------------------------------------------------------------------
// Deployment mode
// ---------------------------------------------------------------------------

// True when served via the Node dev server (npm run dev) or when
// VITE_MATRIXOS_RPC_EXTERNAL=true is set at build time for a Node-backed
// local deployment. False for pure static builds (npm run build).
export const IS_NODE_BACKED =
  import.meta.env.DEV || import.meta.env.VITE_MATRIXOS_RPC_EXTERNAL === 'true'

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
// Emulator error collector (host-side JS errors, separate from MatrixOS logs)
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

// Installs window.onerror / onunhandledrejection collectors.
// Guarded by window.__matrixosRpcErrorsInstalled so multiple initRpc() calls
// (e.g. HMR) don't stack additional wrappers.
function installErrorCollectors() {
  if (window.__matrixosRpcErrorsInstalled) return
  window.__matrixosRpcErrorsInstalled = true

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
// Subscription system (MIDI / HID / Serial push notifications)
// ---------------------------------------------------------------------------

const _subs = { midi: [], hid: [], serial: [] }

function _subscribe(topic, callback) {
  if (!_subs[topic]) return () => {}
  _subs[topic].push(callback)
  return () => {
    _subs[topic] = _subs[topic].filter(cb => cb !== callback)
  }
}

// Install Svelte store observers that forward new events to registered
// subscribers. Guarded so multiple initRpc() calls don't stack watchers.
// Local prev-length counters are captured at init time so the first store
// callback does not re-emit history.
function initSubscriptionBridge() {
  if (window.__matrixosRpcBridgeInstalled) return
  window.__matrixosRpcBridgeInstalled = true

  let prevMidi   = get(midiEvents).length
  let prevHid    = get(hidEvents).length
  let prevSerial = get(serialEvents).length

  midiEvents.subscribe(evts => {
    if (evts.length > prevMidi) {
      evts.slice(prevMidi).forEach(evt => {
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
    prevMidi = evts.length
  })

  hidEvents.subscribe(evts => {
    if (evts.length > prevHid) {
      evts.slice(prevHid).forEach(evt => {
        _subs.hid.forEach(cb => cb({
          timestamp: new Date().toISOString(),
          category: evt.category,
          direction: evt.direction === 'TX' ? 'out' : 'in',
          payload: evt.rawPayload,
        }))
      })
    }
    prevHid = evts.length
  })

  serialEvents.subscribe(evts => {
    if (evts.length > prevSerial) {
      evts.slice(prevSerial).forEach(evt => {
        _subs.serial.forEach(cb => cb({
          timestamp: new Date().toISOString(),
          direction: evt.direction === 'TX' ? 'out' : 'in',
          payload: evt.text,
          encoding: 'utf8',
        }))
      })
    }
    prevSerial = evts.length
  })
}

// ---------------------------------------------------------------------------
// NVS hash parsing
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
// RPC method handlers
// ---------------------------------------------------------------------------

const handlers = {

  // ---- Session ----

  'session.status': () => getSessionStatus({
    sessionId: SESSION_ID,
    protocolVersion: PROTOCOL_VERSION,
  }),

  'session.ping': () => pingSession(),

  'session.reset': () => resetSession(),

  // ---- Runtime ----

  'runtime.getState': () => getRuntimeState(),

  'runtime.getAppState': () => getRuntimeAppState(),

  // ---- Input ----

  'input.list': () => ({ inputs: listInputs() }),

  'input.execute': (params) => {
    const events = params?.events
    if (!Array.isArray(events) || events.length === 0) {
      return { __error: ERR.INVALID_PARAMS }
    }
    const accepted = executeInputEvents(events)
    return { ok: true, accepted }
  },

  'input.get': () => ({ activeInputs: getActiveInputs() }),

  // ---- LED ----

  'led.getFrame': () => getLedFrame(),

  'led.get': (params) => getLed(params),

  // ---- Logs ----

  // Filters: last (count), since (ISO string), to (ISO string), level (string | string[])
  'log.get': (params) => {
    let entries = get(logMessages).map(m => ({
      timestamp: m.isoTimestamp || m.timestamp,
      level: m.level === 'warn' ? 'warning' : m.level,
      tag: m.tag || 'Unknown',
      text: m.text,
      _iso: m.isoTimestamp || null,
    }))

    if (params?.level) {
      const levels = Array.isArray(params.level) ? params.level : [params.level]
      // Map 'warn' → 'warning' for matching convenience
      const normalised = levels.map(l => l === 'warn' ? 'warning' : l)
      entries = entries.filter(e => normalised.includes(e.level))
    }

    if (params?.since) {
      const since = new Date(params.since).getTime()
      if (!isNaN(since)) {
        entries = entries.filter(e => e._iso && new Date(e._iso).getTime() >= since)
      }
    }

    if (params?.to) {
      const to = new Date(params.to).getTime()
      if (!isNaN(to)) {
        entries = entries.filter(e => e._iso && new Date(e._iso).getTime() <= to)
      }
    }

    if (typeof params?.last === 'number') {
      entries = entries.slice(-params.last)
    }

    // Strip internal _iso field before returning
    return { entries: entries.map(({ _iso, ...e }) => e) }
  },

  // Host-side JS errors — separate from MatrixOS runtime logs
  'emulator.getErrors': (params) => {
    let entries = [..._emulatorErrors]

    if (params?.severity) {
      const sev = Array.isArray(params.severity) ? params.severity : [params.severity]
      entries = entries.filter(e => sev.includes(e.severity))
    }
    if (params?.since) {
      const since = new Date(params.since).getTime()
      if (!isNaN(since)) entries = entries.filter(e => new Date(e.timestamp).getTime() >= since)
    }
    if (params?.to) {
      const to = new Date(params.to).getTime()
      if (!isNaN(to)) entries = entries.filter(e => new Date(e.timestamp).getTime() <= to)
    }
    if (typeof params?.last === 'number') {
      entries = entries.slice(-params.last)
    }

    return { entries }
  },

  // ---- MIDI ----

  'midi.listPorts': () => {
    // Synth (0x8000) is excluded per product direction — it is an internal port,
    // not a physical or logical MIDI port the user should address directly.
    const EXCLUDED = new Set([0x8000])
    const ports = get(midiPorts)
      .filter(p => !EXCLUDED.has(p.id))
      .map(p => ({
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
    const unsub = _subscribe('midi', notifyFn)
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
    const unsub = _subscribe('hid', notifyFn)
    return { ok: true, __cleanup: unsub }
  },

  // ---- Serial ----

  'serial.send': (params) => {
    const payload = params?.payload
    if (typeof payload !== 'string') return { __error: ERR.INVALID_PARAMS }

    const encoding = params.encoding ?? 'utf8'
    const ok = encoding === 'hex' ? sendSerialHex(payload) : sendSerialText(payload)
    return ok ? { ok: true } : { __error: ERR.UNSUPPORTED }
  },

  'serial.subscribe': (params, notifyFn) => {
    if (typeof notifyFn !== 'function') return { __error: ERR.INVALID_PARAMS }
    const unsub = _subscribe('serial', notifyFn)
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
    return {
      text,
      hash: '0x' + computeNvsHash(text).toString(16).padStart(8, '0').toUpperCase(),
    }
  },
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
// Public API
// ---------------------------------------------------------------------------

export const rpcApi = {
  call: dispatch,
  subscribe: _subscribe,
  computeNvsHash,
  nvsHashHex,
  isNodeBacked: IS_NODE_BACKED,
  get sessionId() { return SESSION_ID },
  get protocolVersion() { return PROTOCOL_VERSION },
}

// ---------------------------------------------------------------------------
// Init — idempotent; safe to call on remount or HMR
// ---------------------------------------------------------------------------

export function initRpc() {
  // Guard via window flags so re-entrant calls (HMR, StrictMode remount)
  // don't stack error collectors or store subscriptions
  installErrorCollectors()
  initSubscriptionBridge()

  // Only expose the API object in the Node-backed / dev mode.
  // Static builds must not present the JSON-RPC API as available.
  if (IS_NODE_BACKED) {
    window.matrixosRpc = rpcApi
    if (!window.__matrixosRpcReady) {
      window.__matrixosRpcReady = true
      console.info('[MystrixSim] JSON-RPC ready. window.matrixosRpc.call(method, params)')
      console.info('[MystrixSim] Note: transport is in-page only. The local WebSocket server is status-only.')
    }
  } else {
    // Static build — do not expose the API. Delete any stale reference from HMR.
    delete window.matrixosRpc
    if (!window.__matrixosRpcReady) {
      window.__matrixosRpcReady = true
      console.info('[MystrixSim] Static build: JSON-RPC API not exposed (window.matrixosRpc unavailable).')
    }
  }
}

