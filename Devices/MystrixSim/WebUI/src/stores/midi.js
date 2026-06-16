// MIDI event store for MystrixSim dashboard
import { writable, get } from 'svelte/store'
import { sendMidiRaw, sendMidiNote, sendMidiCC, sendMidiProgramChange } from '../handles/midi.js'

let counter = 0
const MAX_EVENTS = 500
const MIDI_PORT_USB = 0x0100
const MIDI_PORT_OS = 0xF000
const MIDI_PORT_EACH_CLASS = 0x0000
const MIDI_PORT_ALL = 0x0001
const MIDI_PREF_KEY = 'matrixos-web-midi-routing'
const SYNTH_PREF_KEY = 'matrixos-web-midi-synths'

export const midiEvents = writable([])
export const midiConnected = writable(false)
export const webMidiState = writable({
  supported: typeof navigator !== 'undefined' && typeof navigator.requestMIDIAccess === 'function',
  requested: false,
  ready: false,
  error: '',
})
export const webMidiInputs = writable([])
export const webMidiOutputs = writable([])
export const midiForwarding = writable(loadForwardingPrefs())
export const synthBindings = writable(loadSynthPrefs())

// Known MIDI ports — names follow OS-canonical definitions (OS/Framework/Midi/MidiPacket.h)
export const midiPorts = writable([
  { id: 0x0100, name: 'USB MIDI', short: 'USB' },
  { id: 0x0200, name: 'Midi Port', short: 'Phys' },
  { id: 0x0300, name: 'Bluetooth', short: 'BT' },
  { id: 0xF000, name: 'MatrixOS', short: 'OS' },
])

let midiAccess = null
let activeNotes = new Map()
let audioCtx = null

export const synthPresets = [
  { id: 'warm-pad', name: 'Warm Pad', wave: 'sawtooth', attack: 0.08, release: 0.5, gain: 0.14, detune: -5 },
  { id: 'soft-sine', name: 'Soft Sine', wave: 'sine', attack: 0.012, release: 0.22, gain: 0.18, detune: 0 },
  { id: 'square-lead', name: 'Square Lead', wave: 'square', attack: 0.006, release: 0.16, gain: 0.13, detune: 0 },
  { id: 'triangle-bell', name: 'Triangle Bell', wave: 'triangle', attack: 0.003, release: 0.75, gain: 0.16, detune: 12 },
  { id: 'bass-pulse', name: 'Bass Pulse', wave: 'square', attack: 0.004, release: 0.18, gain: 0.18, detune: -12 },
  { id: 'pluck', name: 'Pluck', wave: 'sawtooth', attack: 0.002, release: 0.12, gain: 0.13, detune: 7 },
]

function loadJson(key, fallback) {
  if (typeof window === 'undefined') return fallback
  try {
    const raw = window.localStorage.getItem(key)
    if (!raw) return fallback
    return JSON.parse(raw)
  } catch {
    return fallback
  }
}

function loadForwardingPrefs() {
  const prefs = {
    inputEnabled: false,
    outputEnabled: false,
    selectedInputId: 'none',
    selectedOutputId: 'none',
    ...loadJson(MIDI_PREF_KEY, {}),
  }
  return prefs
}

function loadSynthPrefs() {
  const loaded = loadJson(SYNTH_PREF_KEY, null)
  if (Array.isArray(loaded)) return loaded
  return [
    { id: cryptoId(), channel: 1, preset: 'warm-pad', enabled: false, label: 'Synth 1' },
  ]
}

function cryptoId() {
  if (typeof crypto !== 'undefined' && crypto.randomUUID) return crypto.randomUUID()
  return `synth-${Date.now()}-${Math.random().toString(16).slice(2)}`
}

midiForwarding.subscribe((value) => {
  try { window.localStorage.setItem(MIDI_PREF_KEY, JSON.stringify(value)) } catch {}
})

synthBindings.subscribe((value) => {
  try { window.localStorage.setItem(SYNTH_PREF_KEY, JSON.stringify(value)) } catch {}
})

function timestamp() {
  const now = new Date()
  return now.toLocaleTimeString('en-US', {
    hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit'
  }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

// Decode MIDI status byte into human-readable type
function decodeMidiStatus(status) {
  const type = status & 0xF0
  const channel = (status & 0x0F) + 1
  const types = {
    0x80: 'Note Off',
    0x90: 'Note On',
    0xA0: 'Aftertouch',
    0xB0: 'CC',
    0xC0: 'Program',
    0xD0: 'Ch Pressure',
    0xE0: 'Pitch Bend',
    0xF0: 'System',
  }
  if (status === 0xF0) return { type: 'SysEx Start', channel: null }
  if (status === 0xF7) return { type: 'SysEx End', channel: null }
  if (status === 0xF8) return { type: 'Clock', channel: null }
  if (status === 0xFA) return { type: 'Start', channel: null }
  if (status === 0xFB) return { type: 'Continue', channel: null }
  if (status === 0xFC) return { type: 'Stop', channel: null }
  if (status >= 0xF0) return { type: types[0xF0] || `Sys ${status.toString(16)}`, channel: null }
  return { type: types[type] || `0x${type.toString(16)}`, channel }
}

// Format MIDI data for human-readable summary
function formatMidiSummary(status, data0, data1, data2) {
  const s = status & 0xF0
  if (s === 0x90 && data1 > 0) return `Note ${data0} vel ${data1}`
  if (s === 0x80 || (s === 0x90 && data1 === 0)) return `Note ${data0} off`
  if (s === 0xB0) return `CC ${data0} = ${data1}`
  if (s === 0xC0) return `Program ${data0}`
  if (s === 0xD0) return `Pressure ${data0}`
  if (s === 0xE0) return `Bend ${data0 | (data1 << 7)}`
  if (s === 0xA0) return `AT note ${data0} = ${data1}`
  return `${data0.toString(16).padStart(2, '0')} ${data1.toString(16).padStart(2, '0')} ${data2.toString(16).padStart(2, '0')}`
}

// Special port IDs from OS/Framework/Midi/MidiPacket.h
// Synth (0x8000) is intentionally excluded — it is an internal port,
// not a user-addressable physical or logical MIDI port.
const SPECIAL_PORTS = {
  0x0000: { name: 'Each Class', short: 'Each' },
  0x0001: { name: 'All Ports',  short: 'All'  },
  0x0400: { name: 'Wireless',   short: 'WiFi' },
  0x0500: { name: 'RTP MIDI',   short: 'RTP'  },
  0x0600: { name: 'Custom',     short: 'Cust' },
  0xF000: { name: 'MatrixOS',   short: 'OS'   },
  0xFFFF: { name: 'Invalid',    short: '—'    },
}

function portName(portId) {
  const ports = get(midiPorts)
  // Handle USB MIDI sub-ports (0x0100–0x01FF)
  if (portId >= 0x0100 && portId <= 0x01FF) {
    const idx = portId - 0x0100
    return idx === 0 ? 'USB' : `USB${idx + 1}`
  }
  // Handle Physical MIDI sub-ports (0x0200–0x02FF)
  if (portId >= 0x0200 && portId <= 0x02FF) {
    const idx = portId - 0x0200
    return idx === 0 ? 'Phys' : `Phys${idx + 1}`
  }
  // Handle Bluetooth sub-ports (0x0300–0x03FF)
  if (portId >= 0x0300 && portId <= 0x03FF) {
    const idx = portId - 0x0300
    return idx === 0 ? 'BT' : `BT${idx + 1}`
  }
  if (SPECIAL_PORTS[portId]) return SPECIAL_PORTS[portId].short
  const found = ports.find(p => p.id === portId)
  return found ? found.short : `0x${portId.toString(16).toUpperCase()}`
}

export function portLabel(portId) {
  const ports = get(midiPorts)
  if (portId >= 0x0100 && portId <= 0x01FF) {
    const idx = portId - 0x0100
    return idx === 0 ? 'USB MIDI' : `USB MIDI ${idx + 1}`
  }
  if (portId >= 0x0200 && portId <= 0x02FF) {
    const idx = portId - 0x0200
    return idx === 0 ? 'Midi Port' : `Midi Port ${idx + 1}`
  }
  if (portId >= 0x0300 && portId <= 0x03FF) {
    const idx = portId - 0x0300
    return idx === 0 ? 'Bluetooth' : `Bluetooth ${idx + 1}`
  }
  if (SPECIAL_PORTS[portId]) return SPECIAL_PORTS[portId].name
  const found = ports.find(p => p.id === portId)
  return found ? found.name : '0x' + portId.toString(16).toUpperCase().padStart(4, '0')
}

export function portHex(portId) {
  return '0x' + portId.toString(16).toUpperCase().padStart(4, '0')
}

function pushMidiEvent(direction, srcPort, dstPort, status, data0, data1, data2) {
  const decoded = decodeMidiStatus(status)
  const entry = {
    id: counter++,
    timestamp: timestamp(),
    direction: direction === 1 ? 'TX' : 'RX',
    srcPort: portName(srcPort),
    dstPort: portName(dstPort),
    srcPortLabel: portLabel(srcPort),
    dstPortLabel: portLabel(dstPort),
    srcPortHex: portHex(srcPort),
    dstPortHex: portHex(dstPort),
    srcPortId: srcPort,
    dstPortId: dstPort,
    msgType: decoded.type,
    channel: decoded.channel,
    summary: formatMidiSummary(status, data0, data1, data2),
    rawBytes: [status, data0, data1, data2]
      .map(b => b.toString(16).padStart(2, '0').toUpperCase())
      .join(' '),
    status,
    data0,
    data1,
    data2,
  }

  midiEvents.update(events => {
    const trimmed = events.length >= MAX_EVENTS ? events.slice(-MAX_EVENTS + 1) : events
    return [...trimmed, entry]
  })
}

export function clearMidiEvents() {
  midiEvents.set([])
}

function normalizeTappedPacket(status, data0, data1, data2) {
  if (status >= 0x80 && status < 0xF0) {
    return {
      status: data0 & 0xFF,
      data0: data1 & 0x7F,
      data1: data2 & 0x7F,
      data2: 0,
    }
  }

  if (status >= 0xF0 && data0 >= 0xF0) {
    return {
      status: data0 & 0xFF,
      data0: data1 & 0x7F,
      data1: data2 & 0x7F,
      data2: 0,
    }
  }

  return { status, data0, data1, data2 }
}

function midiMessageLength(status) {
  const type = status & 0xF0
  if (status >= 0xF8) return 1
  if (status >= 0xF0) {
    if (status === 0xF1 || status === 0xF3) return 2
    if (status === 0xF2) return 3
    return 1
  }
  if (type === 0xC0 || type === 0xD0) return 2
  return 3
}

function asMidiBytes(status, data0, data1, data2) {
  return [status & 0xFF, data0 & 0x7F, data1 & 0x7F, data2 & 0x7F].slice(0, midiMessageLength(status))
}

function isUsbTarget(dstPort) {
  return dstPort === MIDI_PORT_ALL || dstPort === MIDI_PORT_EACH_CLASS || (dstPort >= 0x0100 && dstPort <= 0x01FF)
}

function refreshWebMidiPorts() {
  if (!midiAccess) {
    webMidiInputs.set([])
    webMidiOutputs.set([])
    return
  }

  const inputs = Array.from(midiAccess.inputs.values()).map(port => ({
    id: port.id,
    name: port.name || 'MIDI Input',
    manufacturer: port.manufacturer || '',
    state: port.state || 'connected',
  }))
  const outputs = Array.from(midiAccess.outputs.values()).map(port => ({
    id: port.id,
    name: port.name || 'MIDI Output',
    manufacturer: port.manufacturer || '',
    state: port.state || 'connected',
  }))

  webMidiInputs.set(inputs)
  webMidiOutputs.set(outputs)
  attachInputHandlers()
}

function attachInputHandlers() {
  if (!midiAccess) return
  midiAccess.inputs.forEach((input) => {
    input.onmidimessage = (event) => {
      const prefs = get(midiForwarding)
      if (!prefs.inputEnabled) return
      if (prefs.selectedInputId === 'none' || prefs.selectedInputId !== input.id) return
      const data = Array.from(event.data || [])
      if (data.length === 0) return
      const status = data[0]
      sendMidiRaw(status, data[1] ?? 0, data[2] ?? 0, 0, MIDI_PORT_OS)
    }
  })
}

export async function requestWebMidiAccess() {
  if (typeof navigator === 'undefined' || typeof navigator.requestMIDIAccess !== 'function') {
    webMidiState.set({ supported: false, requested: true, ready: false, error: 'Web MIDI is not available in this browser.' })
    return false
  }

  webMidiState.update(state => ({ ...state, requested: true, error: '' }))
  try {
    midiAccess = await navigator.requestMIDIAccess({ sysex: false })
    midiAccess.onstatechange = refreshWebMidiPorts
    refreshWebMidiPorts()
    webMidiState.set({ supported: true, requested: true, ready: true, error: '' })
    return true
  } catch (error) {
    webMidiState.set({
      supported: true,
      requested: true,
      ready: false,
      error: error instanceof Error ? error.message : String(error),
    })
    return false
  }
}

export function updateMidiForwarding(patch) {
  midiForwarding.update(value => ({ ...value, ...patch }))
  attachInputHandlers()
}

function forwardToWebMidiOutput(status, data0, data1, data2) {
  if (!midiAccess) return
  const prefs = get(midiForwarding)
  if (!prefs.outputEnabled) return
  const bytes = asMidiBytes(status, data0, data1, data2)
  midiAccess.outputs.forEach((output) => {
    if (prefs.selectedOutputId === 'none' || prefs.selectedOutputId !== output.id) return
    try { output.send(bytes) } catch {}
  })
}

function ensureAudio() {
  if (audioCtx) return audioCtx
  const Ctor = window.AudioContext || window.webkitAudioContext
  if (!Ctor) return null
  audioCtx = new Ctor()
  return audioCtx
}

export function resumeSynthAudio() {
  const ctx = ensureAudio()
  if (ctx?.state === 'suspended') void ctx.resume()
  return Boolean(ctx)
}

function noteFrequency(note) {
  return 440 * Math.pow(2, (note - 69) / 12)
}

function getPreset(id) {
  return synthPresets.find(p => p.id === id) || synthPresets[0]
}

function synthKey(bindingId, channel, note) {
  return `${bindingId}:${channel}:${note}`
}

function stopSynthNote(key, release = 0.18) {
  const voice = activeNotes.get(key)
  if (!voice) return
  activeNotes.delete(key)
  const now = voice.ctx.currentTime
  try {
    voice.gain.gain.cancelScheduledValues(now)
    voice.gain.gain.setValueAtTime(voice.gain.gain.value, now)
    voice.gain.gain.linearRampToValueAtTime(0.0001, now + release)
    voice.osc.stop(now + release + 0.02)
  } catch {}
}

function playSynthNote(binding, channel, note, velocity) {
  const ctx = ensureAudio()
  if (!ctx) return
  if (ctx.state === 'suspended') void ctx.resume()
  const preset = getPreset(binding.preset)
  const key = synthKey(binding.id, channel, note)
  stopSynthNote(key, 0.02)

  const osc = ctx.createOscillator()
  const gain = ctx.createGain()
  const filter = ctx.createBiquadFilter()
  const now = ctx.currentTime
  const level = Math.max(0.02, Math.min(1, velocity / 127)) * preset.gain

  osc.type = preset.wave
  osc.frequency.setValueAtTime(noteFrequency(note), now)
  osc.detune.setValueAtTime(preset.detune || 0, now)
  filter.type = 'lowpass'
  filter.frequency.setValueAtTime(3600, now)
  filter.Q.setValueAtTime(0.8, now)
  gain.gain.setValueAtTime(0.0001, now)
  gain.gain.linearRampToValueAtTime(level, now + preset.attack)

  osc.connect(filter)
  filter.connect(gain)
  gain.connect(ctx.destination)
  osc.start(now)
  activeNotes.set(key, { ctx, osc, gain })
}

function routeToSynths(status, data0, data1) {
  const type = status & 0xF0
  const channel = (status & 0x0F) + 1
  if (type !== 0x80 && type !== 0x90) return
  for (const binding of get(synthBindings)) {
    if (!binding.enabled || binding.channel !== channel) continue
    const key = synthKey(binding.id, channel, data0)
    if (type === 0x90 && data1 > 0) playSynthNote(binding, channel, data0, data1)
    else stopSynthNote(key, getPreset(binding.preset).release)
  }
}

export function addSynthBinding() {
  synthBindings.update(bindings => [
    ...bindings,
    { id: cryptoId(), channel: Math.min(16, bindings.length + 1), preset: 'soft-sine', enabled: true, label: `Synth ${bindings.length + 1}` },
  ])
  resumeSynthAudio()
}

export function updateSynthBinding(id, patch) {
  synthBindings.update(bindings => bindings.map(binding => binding.id === id ? { ...binding, ...patch } : binding))
}

export function auditionSynthBinding(id) {
  const binding = get(synthBindings).find(item => item.id === id)
  if (!binding) return
  const note = 60
  playSynthNote({ ...binding, enabled: true }, binding.channel, note, 96)
  window.setTimeout(() => stopSynthNote(synthKey(binding.id, binding.channel, note), getPreset(binding.preset).release), 220)
}

export function removeSynthBinding(id) {
  for (const key of Array.from(activeNotes.keys())) {
    if (key.startsWith(`${id}:`)) stopSynthNote(key, 0.03)
  }
  synthBindings.update(bindings => bindings.filter(binding => binding.id !== id))
}

export function panicSynth() {
  for (const key of Array.from(activeNotes.keys())) stopSynthNote(key, 0.03)
}

// Install the global tap hook (called from wasm.js init)
export function hookMidiTap() {
  window._matrixos_midi_tap = (direction, srcPort, dstPort, status, data0, data1, data2) => {
    const packet = normalizeTappedPacket(status, data0, data1, data2)
    midiConnected.set(true)
    pushMidiEvent(direction, srcPort, dstPort, packet.status, packet.data0, packet.data1, packet.data2)
    if (direction === 1) {
      if (isUsbTarget(dstPort)) {
        forwardToWebMidiOutput(packet.status, packet.data0, packet.data1, packet.data2)
      }
      routeToSynths(packet.status, packet.data0, packet.data1)
    }
  }
  return () => {
    delete window._matrixos_midi_tap
  }
}

export { sendMidiRaw, sendMidiNote, sendMidiCC, sendMidiProgramChange }
