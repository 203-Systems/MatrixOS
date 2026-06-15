/**
 * Input handle — unified control layer for all input operations.
 *
 * Both the JSON-RPC dispatcher and UI components should use this module
 * rather than calling wasm.js sendGridKey/sendFnKey directly.
 *
 * Supported input IDs:
 *   'function'          — the Function Key
 *   'grid:X,Y'         — a grid key at column X, row Y (0-based, 0..7)
 *   'touchbar:left:N'  — left touchbar key N (0-based, 0..7)
 *   'touchbar:right:N' — right touchbar key N (0-based, 0..7)
 */

import { sendKeyInfoEvent, tickKeypad } from '../stores/wasm.js'

export const INPUT_GRID_SIZE = 8

const INPUT_CLUSTER_FUNCTION = 0
const INPUT_CLUSTER_PRIMARY_GRID = 1
const INPUT_CLUSTER_TOUCHBAR_LEFT = 2
const INPUT_CLUSTER_TOUCHBAR_RIGHT = 3
const INPUT_MEMBER_FUNCTION = 0
const KEYPAD_STATE_PRESSED = 2
const KEYPAD_STATE_RELEASED = 5
const DEFAULT_PRESSURE = 65535
const DEFAULT_VELOCITY = 65535
const RECENT_INPUT_EVENT_LIMIT = 200

const recentInputEvents = []

function recordInputEvent(event) {
  recentInputEvents.push({
    timestamp: new Date().toISOString(),
    ...event,
  })
  if (recentInputEvents.length > RECENT_INPUT_EVENT_LIMIT) {
    recentInputEvents.splice(0, recentInputEvents.length - RECENT_INPUT_EVENT_LIMIT)
  }
}

/**
 * Parse a string input ID into a structured target descriptor.
 * Returns null for unrecognised IDs.
 */
export function parseInputId(id) {
  if (id === 'function') return { kind: 'functionKey' }
  const m = /^grid:(\d+),(\d+)$/.exec(id)
  if (m) {
    const x = parseInt(m[1])
    const y = parseInt(m[2])
    if (x >= 0 && x < INPUT_GRID_SIZE && y >= 0 && y < INPUT_GRID_SIZE) {
      return { kind: 'gridKey', x, y }
    }
  }

  const touchM = /^touchbar:(left|right):(\d+)$/.exec(id)
  if (touchM) {
    const side = touchM[1]
    const index = parseInt(touchM[2], 10)
    if (index >= 0 && index < INPUT_GRID_SIZE) {
      return { kind: 'touchbarKey', side, index }
    }
  }

  return null
}

/**
 * Return the full list of addressable inputs on this device.
 * Function Key first, then grid keys in row-major order.
 */
export function listInputs() {
  const inputs = [
    { id: 'function', label: 'Function Key', kind: 'functionKey' },
  ]
  for (let y = 0; y < INPUT_GRID_SIZE; y++) {
    for (let x = 0; x < INPUT_GRID_SIZE; x++) {
      inputs.push({ id: `grid:${x},${y}`, label: `Grid ${x},${y}`, kind: 'gridKey', x, y })
    }
  }
  for (let i = 0; i < INPUT_GRID_SIZE; i++) {
    inputs.push({ id: `touchbar:left:${i}`, label: `Touchbar Left ${i}`, kind: 'touchbarKey', side: 'left', index: i })
    inputs.push({ id: `touchbar:right:${i}`, label: `Touchbar Right ${i}`, kind: 'touchbarKey', side: 'right', index: i })
  }
  return inputs
}

/**
 * Fire a single Press or Release action on a parsed target.
 * Hold is OS-generated after the hold threshold and cannot be injected.
 */
export function executeInput(target, action) {
  const state = action === 'Press' ? KEYPAD_STATE_PRESSED : KEYPAD_STATE_RELEASED
  const pressure = action === 'Press' ? DEFAULT_PRESSURE : 0
  const velocity = action === 'Press' ? DEFAULT_VELOCITY : 0

  let ok = false
  if (target.kind === 'functionKey') {
    ok = sendKeyInfoEvent(INPUT_CLUSTER_FUNCTION, INPUT_MEMBER_FUNCTION, state, pressure, velocity)
  } else if (target.kind === 'gridKey') {
    const memberId = target.y * INPUT_GRID_SIZE + target.x
    ok = sendKeyInfoEvent(INPUT_CLUSTER_PRIMARY_GRID, memberId, state, pressure, velocity)
  } else if (target.kind === 'touchbarKey') {
    const clusterId = target.side === 'left' ? INPUT_CLUSTER_TOUCHBAR_LEFT : INPUT_CLUSTER_TOUCHBAR_RIGHT
    ok = sendKeyInfoEvent(clusterId, target.index, state, pressure, velocity)
  }
  recordInputEvent({ phase: 'inject', target, action, state, pressure, velocity, ok })
  return ok
}

/**
 * Execute a scheduled sequence of input events.
 *
 * Each event object: { input: string, action: 'Press'|'Release', atMs?: number }
 *   - input    — input ID string (e.g. 'function', 'grid:3,4')
 *   - action   — 'Press' or 'Release' (Hold is not injectable)
 *   - atMs     — delay in ms from now; defaults to 0 (immediate)
 *
 * Returns the count of accepted (valid, schedulable) events.
 */
export function executeInputEvents(events) {
  if (!Array.isArray(events) || events.length === 0) return 0

  const scheduled = []
  for (const evt of events) {
    const target = parseInputId(evt.input)
    if (!target) continue
    const action = evt.action
    if (action !== 'Press' && action !== 'Release') continue

    const atMs = typeof evt.atMs === 'number' ? Math.max(0, evt.atMs) : 0
    scheduled.push({ target, action, atMs })
  }

  if (scheduled.length === 0) return 0

  scheduled.sort((a, b) => a.atMs - b.atMs)
  recordInputEvent({
    phase: 'schedule',
    events: scheduled.map((evt) => ({ target: evt.target, action: evt.action, atMs: evt.atMs })),
  })

  const startedAt = performance.now()
  let nextIndex = 0
  let tickTimer = null
  let finalTickTimer = null

  const runDueEvents = () => {
    const elapsed = performance.now() - startedAt
    while (nextIndex < scheduled.length && scheduled[nextIndex].atMs <= elapsed) {
      const evt = scheduled[nextIndex]
      executeInput(evt.target, evt.action)
      nextIndex++
    }
  }

  const scheduleFinalTick = () => {
    if (finalTickTimer !== null) return
    finalTickTimer = setTimeout(() => {
      finalTickTimer = null
      runDueEvents()
      tickKeypad()
    }, 50)
  }

  const runSchedulerTick = () => {
    runDueEvents()
    tickKeypad()
    if (nextIndex >= scheduled.length && tickTimer !== null) {
      clearInterval(tickTimer)
      tickTimer = null
      scheduleFinalTick()
    }
  }

  runSchedulerTick()
  if (nextIndex < scheduled.length) {
    tickTimer = setInterval(runSchedulerTick, 16)
  } else {
    scheduleFinalTick()
  }

  return scheduled.length
}

/**
 * Release every addressable input to reset simulator input state between automated tests.
 */
export function releaseAllInputs() {
  let released = 0
  for (const input of listInputs()) {
    const target = parseInputId(input.id)
    if (!target) continue
    executeInput(target, 'Release')
    released++
  }
  tickKeypad()
  return released
}

export function getRecentInputEvents(limit = 80) {
  const count = typeof limit === 'number' ? Math.max(0, Math.min(RECENT_INPUT_EVENT_LIMIT, limit)) : 80
  return recentInputEvents.slice(-count)
}

/**
 * Query currently active inputs by polling WASM runtime state.
 * Returns an array of { input, label } objects for each currently held input.
 */
export function getActiveInputs() {
  const mod = window.Module
  const activeInputs = []

  if (mod?._MatrixOS_Wasm_GetFnState?.() !== 0) {
    activeInputs.push({ input: 'function', label: 'Function Key' })
  }

  if (mod?._MatrixOS_Wasm_GetKeypadState && mod?._MatrixOS_Wasm_GetKeypadStateLength) {
    const heap = mod.HEAPU8
    const len = mod._MatrixOS_Wasm_GetKeypadStateLength()
    const ptr = mod._MatrixOS_Wasm_GetKeypadState()
    if (ptr && len && heap) {
      const state = heap.subarray(ptr, ptr + len)
      for (let i = 0; i < state.length; i++) {
        if (state[i]) {
          const x = i % INPUT_GRID_SIZE
          const y = Math.floor(i / INPUT_GRID_SIZE)
          activeInputs.push({ input: `grid:${x},${y}`, label: `Grid ${x},${y}` })
        }
      }
    }
  }

  return activeInputs
}
