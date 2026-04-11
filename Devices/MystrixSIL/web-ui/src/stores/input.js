// Input event tracking store for MystrixSIL dashboard
import { writable } from 'svelte/store'

let counter = 0
const MAX_EVENTS = 300

export const inputEvents = writable([])
export const activeGridKeys = writable(new Set())
export const fnKeyActive = writable(false)

// Runtime-side state (reflects what the OS actually sees, polled from WASM)
export const runtimeGridKeys = writable(new Set())
export const runtimeFnActive = writable(false)

function timestamp() {
  const now = new Date()
  return now.toLocaleTimeString('en-US', {
    hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit'
  }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

export function logInputEvent(type, x, y, pressed, velocity = null) {
  // MystrixSIL binary keypad: press velocity is always 127 (100%), release is 0
  const vel = velocity !== null ? velocity : (type === 'grid' ? (pressed ? 127 : 0) : null)
  const entry = {
    id: counter++,
    type,
    x,
    y,
    pressed,
    velocity: vel,
    timestamp: timestamp()
  }

  inputEvents.update(events => {
    const trimmed = events.length >= MAX_EVENTS ? events.slice(-MAX_EVENTS + 1) : events
    return [...trimmed, entry]
  })

  if (type === 'grid') {
    activeGridKeys.update(keys => {
      const next = new Set(keys)
      const key = `${x},${y}`
      if (pressed) next.add(key)
      else next.delete(key)
      return next
    })
  } else if (type === 'fn') {
    fnKeyActive.set(pressed)
  }
}

export function clearInputEvents() {
  inputEvents.set([])
  activeGridKeys.set(new Set())
  fnKeyActive.set(false)
}

// Called from DevicePanel's renderFrame loop to poll runtime-side keypad state.
export function pollRuntimeState(keypadArray, fnActive) {
  if (keypadArray) {
    const next = new Set()
    for (let i = 0; i < keypadArray.length; i++) {
      if (keypadArray[i]) {
        const x = i % 8
        const y = Math.floor(i / 8)
        next.add(`${x},${y}`)
      }
    }
    runtimeGridKeys.set(next)
  }
  runtimeFnActive.set(!!fnActive)
}
