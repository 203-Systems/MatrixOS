// Input event tracking store for MystrixSIL dashboard
import { writable } from 'svelte/store'

let counter = 0
const MAX_EVENTS = 300

export const inputEvents = writable([])
export const activeGridKeys = writable(new Set())
export const fnKeyActive = writable(false)

function timestamp() {
  const now = new Date()
  return now.toLocaleTimeString('en-US', {
    hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit'
  }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

export function logInputEvent(type, x, y, pressed) {
  const entry = {
    id: counter++,
    type,
    x,
    y,
    pressed,
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
