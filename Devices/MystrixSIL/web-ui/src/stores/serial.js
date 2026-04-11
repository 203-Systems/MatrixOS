// Serial event store for MystrixSIL dashboard
import { writable } from 'svelte/store'

let counter = 0
const MAX_EVENTS = 500

export const serialEvents = writable([])
export const serialConnected = writable(false)

function timestamp() {
  const now = new Date()
  return now.toLocaleTimeString('en-US', {
    hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit'
  }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

// Convert string to hex view
function toHexView(str) {
  return [...str].map(c => c.charCodeAt(0).toString(16).padStart(2, '0').toUpperCase()).join(' ')
}

// Check if string is printable
function isPrintable(str) {
  return /^[\x20-\x7E\t\n\r]*$/.test(str)
}

function pushSerialEvent(direction, text) {
  const entry = {
    id: counter++,
    timestamp: timestamp(),
    direction: direction === 1 ? 'TX' : 'RX',
    text,
    hexView: toHexView(text),
    printable: isPrintable(text),
  }

  serialEvents.update(events => {
    const trimmed = events.length >= MAX_EVENTS ? events.slice(-MAX_EVENTS + 1) : events
    return [...trimmed, entry]
  })
}

export function clearSerialEvents() {
  serialEvents.set([])
}

// Install the global tap hook
export function hookSerialTap() {
  window._matrixos_serial_tap = (direction, text) => {
    serialConnected.set(true)
    pushSerialEvent(direction, text)
  }
  return () => {
    delete window._matrixos_serial_tap
  }
}

// WASM serial write
export function sendSerialText(text) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_SerialWrite) return false
  const encoder = new TextEncoder()
  const bytes = encoder.encode(text)
  const ptr = mod._malloc(bytes.length + 1)
  mod.HEAPU8.set(bytes, ptr)
  mod.HEAPU8[ptr + bytes.length] = 0 // null terminator
  mod._MatrixOS_Wasm_SerialWrite(ptr, bytes.length)
  mod._free(ptr)
  return true
}

export function sendSerialHex(hexString) {
  // Parse hex string like "48 65 6C 6C 6F" into bytes, then send as text
  const bytes = hexString.trim().split(/\s+/).map(h => parseInt(h, 16)).filter(n => !isNaN(n))
  const text = String.fromCharCode(...bytes)
  return sendSerialText(text)
}
