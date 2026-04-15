// Serial event store for MystrixSim dashboard
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

export { sendSerialText, sendSerialHex } from '../handles/serial.js'
