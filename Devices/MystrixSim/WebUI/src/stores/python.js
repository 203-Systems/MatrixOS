// Dedicated Python I/O event store for MystrixSim.
import { writable } from 'svelte/store'

let counter = 0
const MAX_EVENTS = 1000
const MERGE_WINDOW_MS = 40

export const pythonEvents = writable([])
export const pythonPanelSessionStartId = writable(null)
export const pythonPanelUploadedScript = writable(null)

function timestamp() {
  const now = new Date()
  return now.toLocaleTimeString('en-US', {
    hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit',
  }) + '.' + String(now.getMilliseconds()).padStart(3, '0')
}

function normalizeMode(mode) {
  if (mode === 1 || mode === 'repl') return 'repl'
  if (mode === 2 || mode === 'app') return 'app'
  return 'unknown'
}

function pushPythonEvent(modeValue, text) {
  const mode = normalizeMode(modeValue)
  const receivedAtMs = Date.now()

  pythonEvents.update((events) => {
    const trimmed = events.length >= MAX_EVENTS ? events.slice(-MAX_EVENTS + 1) : events
    const last = trimmed[trimmed.length - 1]

    if (last && last.mode === mode && receivedAtMs - last.receivedAtMs <= MERGE_WINDOW_MS) {
      const merged = {
        ...last,
        text: `${last.text}${text}`,
        timestamp: timestamp(),
        receivedAtMs,
      }
      return [...trimmed.slice(0, -1), merged]
    }

    return [
      ...trimmed,
      {
        id: counter++,
        timestamp: timestamp(),
        receivedAtMs,
        mode,
        text,
      },
    ]
  })
}

export function clearPythonEvents() {
  pythonEvents.set([])
}

export function resetPythonPanelState() {
  pythonPanelSessionStartId.set(null)
  pythonPanelUploadedScript.set(null)
}

export function hookPythonTap() {
  window._matrixos_python_tap = (mode, text) => {
    pushPythonEvent(mode, String(text ?? ''))
  }

  return () => {
    delete window._matrixos_python_tap
  }
}
