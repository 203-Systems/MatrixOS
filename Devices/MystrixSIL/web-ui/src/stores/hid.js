// HID event store for MystrixSIL dashboard
import { writable } from 'svelte/store'

let counter = 0
const MAX_EVENTS = 500

export const hidEvents = writable([])
export const hidConnected = writable(false)

const CATEGORIES = ['Keyboard', 'Gamepad', 'RawHID']
const DIRECTIONS = ['RX', 'TX']

function timestamp() {
  const now = new Date()
  return now.toLocaleTimeString('en-US', {
    hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit'
  }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

// Decode keyboard report to human-readable
function decodeKeyboard(bytes) {
  const modifiers = bytes[0]
  const keys = bytes.slice(2).filter(k => k !== 0)
  const modNames = []
  if (modifiers & 0x01) modNames.push('LCtrl')
  if (modifiers & 0x02) modNames.push('LShift')
  if (modifiers & 0x04) modNames.push('LAlt')
  if (modifiers & 0x08) modNames.push('LGui')
  if (modifiers & 0x10) modNames.push('RCtrl')
  if (modifiers & 0x20) modNames.push('RShift')
  if (modifiers & 0x40) modNames.push('RAlt')
  if (modifiers & 0x80) modNames.push('RGui')
  const parts = []
  if (modNames.length) parts.push(modNames.join('+'))
  if (keys.length) parts.push(`keys:[${keys.map(k => '0x' + k.toString(16)).join(',')}]`)
  return parts.join(' ') || 'No keys'
}

// Decode gamepad data
function decodeGamepad(buttons, dpad, xAxis, yAxis, zAxis, rxAxis, ryAxis, rzAxis) {
  const parts = []
  if (buttons) {
    const pressed = []
    for (let i = 0; i < 32; i++) {
      if (buttons & (1 << i)) pressed.push(i)
    }
    parts.push(`btn:[${pressed.join(',')}]`)
  }
  if (dpad) parts.push(`dpad:${dpad}`)
  if (xAxis || yAxis) parts.push(`xy:(${xAxis},${yAxis})`)
  if (zAxis) parts.push(`z:${zAxis}`)
  if (rxAxis || ryAxis || rzAxis) parts.push(`r:(${rxAxis},${ryAxis},${rzAxis})`)
  return parts.join(' ') || 'Idle'
}

function pushHidEvent(category, direction, payload, summary) {
  const entry = {
    id: counter++,
    timestamp: timestamp(),
    category: CATEGORIES[category] || `Unknown(${category})`,
    direction: DIRECTIONS[direction] || `?${direction}`,
    summary,
    rawPayload: payload,
  }

  hidEvents.update(events => {
    const trimmed = events.length >= MAX_EVENTS ? events.slice(-MAX_EVENTS + 1) : events
    return [...trimmed, entry]
  })
}

export function clearHidEvents() {
  hidEvents.set([])
}

// Install the global tap hook
export function hookHidTap() {
  window._matrixos_hid_tap = (category, direction, ...args) => {
    hidConnected.set(true)

    if (category === 0) {
      // Keyboard: args = [key0, key1, key2, key3, key4, key5, key6, key7]
      const bytes = args.slice(0, 8).map(Number)
      const payload = bytes.map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' ')
      const summary = decodeKeyboard(bytes)
      pushHidEvent(0, direction, payload, summary)
    } else if (category === 1) {
      // Gamepad: args = [buttons, dpad, xAxis, yAxis, zAxis, rxAxis, ryAxis, rzAxis]
      const [buttons, dpad, xAxis, yAxis, zAxis, rxAxis, ryAxis, rzAxis] = args.map(Number)
      const payload = `btn:0x${(buttons >>> 0).toString(16)} dpad:${dpad} axes:${xAxis},${yAxis},${zAxis},${rxAxis},${ryAxis},${rzAxis}`
      const summary = decodeGamepad(buttons, dpad, xAxis, yAxis, zAxis, rxAxis, ryAxis, rzAxis)
      pushHidEvent(1, direction, payload, summary)
    } else if (category === 2) {
      // RawHID: args[0] = array of bytes
      const bytes = Array.isArray(args[0]) ? args[0] : args
      const payload = bytes.map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' ')
      const summary = `${bytes.length} bytes`
      pushHidEvent(2, direction, payload, summary)
    }
  }
  return () => {
    delete window._matrixos_hid_tap
  }
}

// WASM RawHID injection
export function sendRawHid(bytes) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_RawHidInject || !mod.HEAPU8) return false
  const len = Math.min(bytes.length, 32)
  const ptr = mod._malloc(len)
  mod.HEAPU8.set(bytes.slice(0, len), ptr)
  mod._MatrixOS_Wasm_RawHidInject(ptr, len)
  mod._free(ptr)
  return true
}
