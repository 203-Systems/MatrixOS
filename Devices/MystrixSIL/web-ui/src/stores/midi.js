// MIDI event store for MystrixSIL dashboard
import { writable, get } from 'svelte/store'

let counter = 0
const MAX_EVENTS = 500

export const midiEvents = writable([])
export const midiConnected = writable(false)

// Known MIDI ports (populated when runtime provides them)
export const midiPorts = writable([
  { id: 0x0100, name: 'USB' },
  { id: 0x0200, name: 'Physical' },
  { id: 0x0300, name: 'Bluetooth' },
  { id: 0x8000, name: 'Synth' },
  { id: 0xF000, name: 'OS' },
])

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

function portName(portId) {
  const ports = get(midiPorts)
  const found = ports.find(p => p.id === portId)
  return found ? found.name : `0x${portId.toString(16)}`
}

function pushMidiEvent(direction, srcPort, dstPort, status, data0, data1, data2) {
  const decoded = decodeMidiStatus(status)
  const entry = {
    id: counter++,
    timestamp: timestamp(),
    direction: direction === 1 ? 'TX' : 'RX',
    srcPort: portName(srcPort),
    dstPort: portName(dstPort),
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

// Install the global tap hook (called from wasm.js init)
export function hookMidiTap() {
  window._matrixos_midi_tap = (direction, srcPort, dstPort, status, data0, data1, data2) => {
    midiConnected.set(true)
    pushMidiEvent(direction, srcPort, dstPort, status, data0, data1, data2)
  }
  return () => {
    delete window._matrixos_midi_tap
  }
}

// WASM send helpers
export function sendMidiNote(channel, note, velocity, targetPort) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_MidiInject) return false
  const status = velocity > 0 ? (0x90 | (channel & 0x0F)) : (0x80 | (channel & 0x0F))
  mod._MatrixOS_Wasm_MidiInject(status, note & 0x7F, velocity & 0x7F, 0, targetPort)
  return true
}

export function sendMidiCC(channel, controller, value, targetPort) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_MidiInject) return false
  const status = 0xB0 | (channel & 0x0F)
  mod._MatrixOS_Wasm_MidiInject(status, controller & 0x7F, value & 0x7F, 0, targetPort)
  return true
}

export function sendMidiProgramChange(channel, program, targetPort) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_MidiInject) return false
  const status = 0xC0 | (channel & 0x0F)
  mod._MatrixOS_Wasm_MidiInject(status, program & 0x7F, 0, 0, targetPort)
  return true
}
