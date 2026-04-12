/**
 * Input handle — unified control layer for all input operations.
 *
 * Both the JSON-RPC dispatcher and UI components should use this module
 * rather than calling wasm.js sendGridKey/sendFnKey directly.
 *
 * Supported input IDs:
 *   'function'          — the Function Key
 *   'grid:X,Y'         — a grid key at column X, row Y (0-based, 0..7)
 */

import { sendGridKey, sendFnKey } from '../stores/wasm.js'

export const INPUT_GRID_SIZE = 8

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
  return inputs
}

/**
 * Fire a single Press or Release action on a parsed target.
 * Hold is OS-generated after the hold threshold and cannot be injected.
 */
export function executeInput(target, action) {
  if (target.kind === 'functionKey') {
    sendFnKey(action === 'Press')
  } else if (target.kind === 'gridKey') {
    sendGridKey(target.x, target.y, action === 'Press')
  }
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

  let accepted = 0
  for (const evt of events) {
    const target = parseInputId(evt.input)
    if (!target) continue
    const action = evt.action
    if (action !== 'Press' && action !== 'Release') continue

    const atMs = typeof evt.atMs === 'number' ? Math.max(0, evt.atMs) : 0
    if (atMs === 0) {
      executeInput(target, action)
    } else {
      setTimeout(() => executeInput(target, action), atMs)
    }
    accepted++
  }
  return accepted
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
