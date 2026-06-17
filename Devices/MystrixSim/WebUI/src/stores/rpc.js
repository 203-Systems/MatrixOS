/**
 * MystrixSim JSON-RPC 2.0 dispatcher
 *
 * Transport: in-page function call.
 *   window.matrixosRpc.call(method, params) → Promise<result>
 *   window.matrixosRpc.subscribe(topic, callback) → unsubscribe()
 *
 * Implements the MystrixSim JSON-RPC surface, including NVS and virtual
 * filesystem storage helpers.
 *
 * Deployment modes:
 *   IS_NODE_BACKED = true  → dev server (npm run dev) or a build with
 *                            VITE_MATRIXOS_RPC_EXTERNAL=true.
 *                            window.matrixosRpc is exposed in this tab.
 *                            This tab also bridges external WebSocket JSON-RPC
 *                            requests to the live runtime.
 *   IS_NODE_BACKED = false → static browser-only build (npm run build).
 *                            window.matrixosRpc is NOT exposed.
 *                            JSON-RPC shows as Unavailable in the UI.
 */

import { get } from 'svelte/store'
import { logMessages } from './logs.js'
import { midiEvents, midiPorts } from './midi.js'
import { hidEvents } from './hid.js'
import { serialEvents } from './serial.js'
import { clearPythonEvents, pythonEvents } from './python.js'
import { nvsEntries, writeNvsEntry, computeNvsHash, nvsHashHex, refreshNvs } from './storage.js'
import { listInputs, executeInputEvents, getActiveInputs, releaseAllInputs, getRecentInputEvents } from '../handles/input.js'
import { getLedFrame, getLed } from '../handles/led.js'
import {
  sendMidiNote,
  sendMidiNoteToPort,
  sendMidiCC,
  sendMidiCCToPort,
  sendMidiProgramChange,
  sendMidiProgramChangeToPort,
  sendMidiSysEx,
  sendMidiSysExToPort,
} from '../handles/midi.js'
import { sendRawHid } from '../handles/hid.js'
import { sendSerialText, sendSerialHex } from '../handles/serial.js'
import {
  hasPythonApp,
  isPythonAppActive,
  getPythonSessionMode,
  getPythonDebugInfo,
  enterPythonRepl,
  clearStagedPythonScripts,
  stagePythonScript,
  stagePythonFiles,
  runStagedPythonScript,
  stopPythonApp,
  sendPythonInput,
} from '../handles/python.js'
import {
  isFilesystemMounted,
  listFilesystemDirectory,
  readFilesystemFile,
  writeFilesystemFile,
  deleteFilesystemPath,
  makeFilesystemDirectory,
} from '../handles/filesystem.js'
import {
  getSessionStatus,
  pingSession,
  resetSession,
  getRuntimeState,
  getRuntimeAppState,
} from '../handles/session.js'
import {
  getApplicationState,
  launchApplication,
} from '../handles/application.js'
import {
  connectPhysicalDevice,
  disconnectPhysicalDevice,
  getPhysicalDeviceSnapshot,
  initializePhysicalBridge,
  listPhysicalHidDevices,
} from './physicalDevice.js'

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

const _subs = { midi: [], hid: [], serial: [], python: [] }

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
  let prevPython = get(pythonEvents).length

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

  pythonEvents.subscribe(evts => {
    if (evts.length > prevPython) {
      evts.slice(prevPython).forEach(evt => {
        _subs.python.forEach(cb => cb({
          timestamp: new Date().toISOString(),
          mode: evt.mode,
          payload: evt.text,
        }))
      })
    }
    prevPython = evts.length
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

function decodeRpcBytes(value, encoding = 'utf8') {
  if (Array.isArray(value)) {
    const bytes = value.map((item) => Number(item) & 0xFF)
    if (bytes.some((item) => Number.isNaN(item))) return null
    return new Uint8Array(bytes)
  }

  if (typeof value !== 'string') return null

  if (encoding === 'hex') {
    const bytes = value
      .trim()
      .split(/\s+/)
      .filter(Boolean)
      .map((item) => parseInt(item, 16))
    return bytes.some((item) => Number.isNaN(item)) ? null : new Uint8Array(bytes)
  }

  if (encoding === 'base64') {
    try {
      return Uint8Array.from(atob(value), (char) => char.charCodeAt(0))
    } catch {
      return null
    }
  }

  if (encoding === 'utf8') {
    return new TextEncoder().encode(value)
  }

  return null
}

function encodeRpcBytes(bytes, encoding = 'base64') {
  const data = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes ?? [])

  if (encoding === 'hex') {
    return Array.from(data).map((item) => item.toString(16).padStart(2, '0').toUpperCase()).join(' ')
  }

  if (encoding === 'utf8') {
    return new TextDecoder().decode(data)
  }

  return btoa(String.fromCharCode(...data))
}

function parseMidiTargetPort(params, fallback = 0x0100) {
  const rawTarget = params?.targetPort
  const value = typeof rawTarget === 'string'
    ? (rawTarget.trim().match(/^0x/i) ? parseInt(rawTarget, 16) : Number(rawTarget))
    : (typeof rawTarget === 'number' ? rawTarget : fallback)
  return Number.isInteger(value) && value >= 0 && value <= 0xFFFF ? value : null
}

function hasMidiTargetPort(params) {
  return params && Object.prototype.hasOwnProperty.call(params, 'targetPort')
}

function decodeMidiSysExPayload(params) {
  const payload = params?.payload
  if (!payload) return null

  let bytes
  if (Array.isArray(payload)) {
    bytes = payload.map(byte => Number(byte))
  } else if ((params.encoding ?? 'hex') === 'hex') {
    const tokens = String(payload).trim().split(/\s+/).filter(Boolean)
    if (tokens.some(token => !/^[0-9a-fA-F]{1,2}$/.test(token))) return null
    bytes = tokens.map(h => parseInt(h, 16))
  } else {
    bytes = [...new TextEncoder().encode(String(payload))]
  }

  if (!Array.isArray(bytes) || bytes.some(byte => !Number.isFinite(byte) || byte < 0 || byte > 0xFF)) {
    return null
  }
  return bytes
}

function sendMidiMessage(message, senders, targetPort = null) {
  if (!message) return false

  const ch = typeof message.channel === 'number' ? (message.channel - 1) & 0x0F : 0
  const kind = message.kind?.toLowerCase()
  const args = targetPort === null ? [] : [targetPort]

  if (kind === 'noteon' || kind === 'note_on') {
    return senders.note(ch, message.note ?? 60, message.velocity ?? 100, ...args)
  }
  if (kind === 'noteoff' || kind === 'note_off') {
    return senders.note(ch, message.note ?? 60, 0, ...args)
  }
  if (kind === 'cc' || kind === 'controlchange' || kind === 'control_change') {
    return senders.cc(ch, message.controller ?? 0, message.value ?? 0, ...args)
  }
  if (kind === 'programchange' || kind === 'program_change') {
    return senders.program(ch, message.program ?? 0, ...args)
  }

  return false
}

async function waitForPythonMode(mode, timeoutMs = 5000) {
  const deadline = Date.now() + timeoutMs

  while (Date.now() < deadline) {
    if (isPythonAppActive() && getPythonSessionMode() === mode) return true
    await new Promise((resolve) => setTimeout(resolve, 50))
  }

  return isPythonAppActive() && getPythonSessionMode() === mode
}

async function waitForPythonLaunch(mode, timeoutMs = 5000, startEventCount = get(pythonEvents).length) {
  const deadline = Date.now() + timeoutMs
  let sawActive = false

  while (Date.now() < deadline) {
    const active = isPythonAppActive()
    const currentMode = getPythonSessionMode()

    if (active && currentMode === mode) return { ok: true, mode: currentMode }
    if (active) sawActive = true

    if (get(pythonEvents).length > startEventCount) {
      return { ok: true, mode: currentMode, completed: !active }
    }

    if (sawActive && !active) {
      return { ok: true, mode: currentMode, completed: true }
    }

    await new Promise((resolve) => setTimeout(resolve, 50))
  }

  return { ok: false, mode: getPythonSessionMode() }
}

async function waitForPythonIdle(timeoutMs = 2000) {
  const deadline = Date.now() + timeoutMs

  while (Date.now() < deadline) {
    if (!isPythonAppActive()) return true
    await new Promise((resolve) => setTimeout(resolve, 50))
  }

  return !isPythonAppActive()
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

  // ---- Applications ----

  'application.list': () => getApplicationState(),

  'application.launch': (params) => {
    const appId = typeof params?.appId === 'string'
      ? parseInt(params.appId.replace(/^0x/i, ''), 16)
      : Number(params?.appId)
    if (!Number.isFinite(appId)) return { __error: ERR.INVALID_PARAMS }
    return launchApplication(appId) ? { ok: true } : { __error: ERR.UNSUPPORTED }
  },

  // ---- Physical Hardware ----

  'physical.state': () => getPhysicalDeviceSnapshot(),

  'physical.list': async () => {
    await initializePhysicalBridge()
    return { devices: await listPhysicalHidDevices() }
  },

  'physical.connect': async (params) => {
    await initializePhysicalBridge()
    await connectPhysicalDevice({ useAuthorized: params?.useAuthorized !== false })
    return getPhysicalDeviceSnapshot()
  },

  'physical.disconnect': async () => {
    await disconnectPhysicalDevice()
    return getPhysicalDeviceSnapshot()
  },

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

  'input.releaseAll': () => ({ ok: true, released: releaseAllInputs() }),

  'input.get': (params) => ({
    activeInputs: getActiveInputs(),
    recentEvents: getRecentInputEvents(params?.last ?? 80),
  }),

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
    if (hasMidiTargetPort(params)) return { __error: ERR.INVALID_PARAMS }
    const ok = sendMidiMessage(params?.message, {
      note: sendMidiNote,
      cc: sendMidiCC,
      program: sendMidiProgramChange,
    })
    return ok ? { ok: true } : { __error: ERR.INVALID_PARAMS }
  },

  'midi.sendToPort': (params) => {
    const targetPort = parseMidiTargetPort(params, 0x0100)
    if (!Number.isFinite(targetPort)) return { __error: ERR.INVALID_PARAMS }
    const ok = sendMidiMessage(params?.message, {
      note: sendMidiNoteToPort,
      cc: sendMidiCCToPort,
      program: sendMidiProgramChangeToPort,
    }, targetPort)
    return ok ? { ok: true } : { __error: ERR.INVALID_PARAMS }
  },

  'midi.sendSysEx': (params) => {
    if (hasMidiTargetPort(params)) return { __error: ERR.INVALID_PARAMS }
    const bytes = decodeMidiSysExPayload(params)
    if (bytes === null) return { __error: ERR.INVALID_PARAMS }
    const ok = sendMidiSysEx(bytes)
    return ok ? { ok: true } : { __error: ERR.INVALID_PARAMS }
  },

  'midi.sendSysExToPort': (params) => {
    const bytes = decodeMidiSysExPayload(params)
    if (bytes === null) return { __error: ERR.INVALID_PARAMS }
    const targetPort = parseMidiTargetPort(params, 0x0100)
    if (!Number.isFinite(targetPort)) return { __error: ERR.INVALID_PARAMS }
    const ok = sendMidiSysExToPort(bytes, targetPort)
    return ok ? { ok: true } : { __error: ERR.INVALID_PARAMS }
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

  // ---- Python ----

  'python.status': () => ({
    available: hasPythonApp(),
    active: isPythonAppActive(),
    mode: getPythonSessionMode(),
    debug: getPythonDebugInfo(),
  }),

  'python.debug': () => getPythonDebugInfo(),

  'python.enterRepl': async (params) => {
    if (isPythonAppActive()) {
      return { __error: { ...ERR.UNSUPPORTED, detail: 'Python is already active.' } }
    }
    clearPythonEvents()
    if (!enterPythonRepl()) return { __error: ERR.UNSUPPORTED }

    const ok = await waitForPythonMode('repl', params?.timeoutMs ?? 5000)
    return ok
      ? { ok: true, mode: getPythonSessionMode() }
      : { __error: { ...ERR.TIMEOUT, detail: 'Python REPL did not become active.' } }
  },

  'python.stage': (params) => {
    const name = params?.name ?? params?.fileName ?? 'remote.py'
    const text = params?.text
    if (typeof name !== 'string' || typeof text !== 'string') return { __error: ERR.INVALID_PARAMS }

    const ok = stagePythonScript(name, text)
    return ok ? { ok: true, name, size: text.length } : { __error: ERR.UNSUPPORTED }
  },

  'python.stageFiles': (params) => {
    const files = params?.files
    const entry = params?.entry
    if (!Array.isArray(files) || files.length === 0) return { __error: ERR.INVALID_PARAMS }

    const normalized = files.map((file) => {
      const name = file?.name ?? file?.fileName
      const text = file?.text
      if (typeof name !== 'string' || typeof text !== 'string') return null
      return { name, text }
    })
    if (normalized.some((file) => file === null)) return { __error: ERR.INVALID_PARAMS }

    let ordered = normalized
    if (typeof entry === 'string') {
      const entryIndex = normalized.findIndex((file) => file.name === entry)
      if (entryIndex < 0) return { __error: ERR.INVALID_PARAMS }
      ordered = normalized.filter((_, index) => index !== entryIndex)
      ordered.push(normalized[entryIndex])
    }

    const ok = stagePythonFiles(ordered)
    return ok ? { ok: true, count: ordered.length, entry: ordered[ordered.length - 1].name } : { __error: ERR.UNSUPPORTED }
  },

  'python.runStaged': async (params) => {
    if (isPythonAppActive() && !(await waitForPythonIdle(params?.idleTimeoutMs ?? 1200))) {
      return { __error: { ...ERR.UNSUPPORTED, detail: 'Python is already active.' } }
    }
    clearPythonEvents()
    const startEventCount = get(pythonEvents).length
    if (!runStagedPythonScript()) return { __error: ERR.UNSUPPORTED }

    const launch = await waitForPythonLaunch('app', params?.timeoutMs ?? 5000, startEventCount)
    return launch.ok
      ? { ok: true, mode: launch.mode, completed: launch.completed === true }
      : { __error: { ...ERR.TIMEOUT, detail: 'Python app did not start.' } }
  },

  'python.runText': async (params) => {
    const name = params?.name ?? params?.fileName ?? 'remote.py'
    const text = params?.text
    if (typeof name !== 'string' || typeof text !== 'string') return { __error: ERR.INVALID_PARAMS }
    if (isPythonAppActive() && !(await waitForPythonIdle(params?.idleTimeoutMs ?? 1200))) {
      return { __error: { ...ERR.UNSUPPORTED, detail: 'Python is already active.' } }
    }
    clearStagedPythonScripts()
    if (!stagePythonScript(name, text)) return { __error: ERR.UNSUPPORTED }
    clearPythonEvents()
    const startEventCount = get(pythonEvents).length
    if (!runStagedPythonScript()) return { __error: ERR.UNSUPPORTED }

    const launch = await waitForPythonLaunch('app', params?.timeoutMs ?? 5000, startEventCount)
    return launch.ok
      ? { ok: true, name, size: text.length, mode: launch.mode, completed: launch.completed === true }
      : { __error: { ...ERR.TIMEOUT, detail: 'Python script did not start.' } }
  },

  'python.stop': async (params) => {
    if (!stopPythonApp()) return { __error: ERR.UNSUPPORTED }
    const ok = await waitForPythonIdle(params?.timeoutMs ?? 1200)
    return ok
      ? { ok: true, mode: getPythonSessionMode() }
      : { __error: { ...ERR.TIMEOUT, detail: 'Python did not stop.' } }
  },

  'python.input': (params) => {
    const text = params?.text
    if (typeof text !== 'string') return { __error: ERR.INVALID_PARAMS }

    const ok = sendPythonInput(text)
    return ok ? { ok: true } : { __error: ERR.UNSUPPORTED }
  },

  'python.getOutput': (params) => {
    let entries = get(pythonEvents).map(evt => ({
      timestamp: evt.timestamp,
      mode: evt.mode,
      text: evt.text,
    }))

    if (params?.mode) {
      entries = entries.filter(evt => evt.mode === params.mode)
    }

    if (typeof params?.last === 'number') {
      entries = entries.slice(-params.last)
    }

    return { entries }
  },

  'python.subscribe': (params, notifyFn) => {
    if (typeof notifyFn !== 'function') return { __error: ERR.INVALID_PARAMS }
    const unsub = _subscribe('python', notifyFn)
    return { ok: true, __cleanup: unsub }
  },

  // ---- Storage / NVS ----

  'storage.nvs.find': (params) => {
    refreshNvs()
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
    refreshNvs()
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

  // ---- Storage / Filesystem ----

  'storage.fs.status': () => ({
    mounted: isFilesystemMounted(),
  }),

  'storage.fs.list': (params) => {
    const path = typeof params?.path === 'string' ? params.path : '/'
    const result = listFilesystemDirectory(path)
    return result?.ok
      ? result
      : { __error: { ...ERR.UNKNOWN_TARGET, detail: result?.error || 'Failed to list directory.' } }
  },

  'storage.fs.read': (params) => {
    const path = params?.path
    const encoding = params?.encoding ?? 'base64'
    if (typeof path !== 'string') return { __error: ERR.INVALID_PARAMS }

    const bytes = readFilesystemFile(path)
    if (bytes == null) return { __error: ERR.UNKNOWN_TARGET }

    return {
      path,
      size: bytes.length,
      encoding,
      data: encodeRpcBytes(bytes, encoding),
    }
  },

  'storage.fs.write': (params) => {
    const path = params?.path
    const encoding = params?.encoding ?? 'utf8'
    const bytes = decodeRpcBytes(params?.data, encoding)
    if (typeof path !== 'string' || bytes == null) return { __error: ERR.INVALID_PARAMS }

    const ok = writeFilesystemFile(path, bytes)
    return ok ? { ok: true, path, size: bytes.length } : { __error: ERR.INTERNAL }
  },

  'storage.fs.delete': (params) => {
    const path = params?.path
    if (typeof path !== 'string') return { __error: ERR.INVALID_PARAMS }

    const ok = deleteFilesystemPath(path)
    return ok ? { ok: true, path } : { __error: ERR.UNKNOWN_TARGET }
  },

  'storage.fs.mkdir': (params) => {
    const path = params?.path
    if (typeof path !== 'string') return { __error: ERR.INVALID_PARAMS }

    const ok = makeFilesystemDirectory(path)
    return ok ? { ok: true, path } : { __error: ERR.INTERNAL }
  },
}

// ---------------------------------------------------------------------------
// Dispatcher
// ---------------------------------------------------------------------------

let _reqCounter = 0
let _externalRpcBridgeSocket = null
let _externalRpcBridgeReconnectHandle = 0

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
    const result = await handler(params, notifyFn)
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

function getExternalRpcWsUrl() {
  const host = window.location.hostname || 'localhost'
  const port = import.meta.env.VITE_MATRIXOS_RPC_PORT || '4002'
  return `ws://${host}:${port}/?role=runtime`
}

function scheduleExternalRpcBridgeReconnect() {
  if (!IS_NODE_BACKED || _externalRpcBridgeReconnectHandle) return

  _externalRpcBridgeReconnectHandle = window.setTimeout(() => {
    _externalRpcBridgeReconnectHandle = 0
    initExternalRpcBridge()
  }, 1000)
}

async function handleExternalRpcBridgeMessage(socket, event) {
  let payload = null
  try {
    payload = JSON.parse(event.data)
  } catch {
    return
  }

  if (payload?.type !== 'rpc_request' || !payload.requestId || typeof payload.method !== 'string') {
    return
  }

  const notifyMethod = payload.method.endsWith('.subscribe')
    ? `${payload.method.slice(0, -'.subscribe'.length)}.event`
    : null

  const response = await dispatch(
    payload.method,
    payload.params ?? {},
    notifyMethod
      ? (params) => {
        if (socket.readyState !== WebSocket.OPEN) return
        socket.send(JSON.stringify({
          type: 'rpc_notification',
          requestId: payload.requestId,
          method: notifyMethod,
          params,
        }))
      }
      : null,
  )
  if (socket.readyState !== WebSocket.OPEN) return

  socket.send(JSON.stringify({
    type: 'rpc_response',
    requestId: payload.requestId,
    response,
  }))
}

function initExternalRpcBridge() {
  if (!IS_NODE_BACKED) return
  if (_externalRpcBridgeSocket && (
    _externalRpcBridgeSocket.readyState === WebSocket.CONNECTING ||
    _externalRpcBridgeSocket.readyState === WebSocket.OPEN
  )) {
    return
  }

  const socket = new WebSocket(getExternalRpcWsUrl(), 'matrixos-runtime')
  _externalRpcBridgeSocket = socket

  socket.addEventListener('message', (event) => {
    void handleExternalRpcBridgeMessage(socket, event)
  })

  socket.addEventListener('close', () => {
    if (_externalRpcBridgeSocket === socket) {
      _externalRpcBridgeSocket = null
    }
    scheduleExternalRpcBridgeReconnect()
  })

  socket.addEventListener('error', () => {})
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
    initExternalRpcBridge()
    if (!window.__matrixosRpcReady) {
      window.__matrixosRpcReady = true
      console.info('[MystrixSim] JSON-RPC ready. window.matrixosRpc.call(method, params)')
      console.info('[MystrixSim] External WebSocket RPC bridge is active while this tab remains open.')
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

