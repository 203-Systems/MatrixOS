/**
 * LED / framebuffer handle layer.
 *
 * Owns framebuffer access, LED id/index resolution, and export helpers for:
 *   - led.getFrame
 *   - led.get
 */

import { INPUT_GRID_SIZE } from './input.js'

const ERR = {
  INVALID_PARAMS: { code: 4001, message: 'Invalid params' },
  UNKNOWN_TARGET: { code: 4002, message: 'Unknown target' },
  UNSUPPORTED: { code: 4003, message: 'Unsupported capability' },
}

export function getFramebufferData() {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_GetFrameBuffer || !mod.HEAPU8) return null
  const ptr = mod._MatrixOS_Wasm_GetFrameBuffer()
  const byteLen = mod._MatrixOS_Wasm_GetFrameBufferByteLength?.() ?? 0
  if (!ptr || !byteLen) return null
  return mod.HEAPU8.subarray(ptr, ptr + byteLen)
}

export function ledAtIndex(data, idx) {
  const base = idx * 4
  return {
    r: data[base],
    g: data[base + 1],
    b: data[base + 2],
    w: data[base + 3],
  }
}

export function ledToHex({ r, g, b, w }) {
  return (
    r.toString(16).padStart(2, '0') +
    g.toString(16).padStart(2, '0') +
    b.toString(16).padStart(2, '0') +
    w.toString(16).padStart(2, '0')
  ).toUpperCase()
}

export function resolveLedIndex(id) {
  const gridM = /^grid:(\d+),(\d+)$/.exec(id)
  const ugM = /^underglow:(\d+)$/.exec(id)

  if (gridM) {
    const x = parseInt(gridM[1], 10)
    const y = parseInt(gridM[2], 10)
    if (x < 0 || x >= INPUT_GRID_SIZE || y < 0 || y >= INPUT_GRID_SIZE) return null
    return y * INPUT_GRID_SIZE + x
  }

  if (ugM) {
    return 64 + parseInt(ugM[1], 10)
  }

  return null
}

export function getLedFrame() {
  const data = getFramebufferData()
  if (!data) return { __error: ERR.UNSUPPORTED }

  const totalLeds = Math.floor(data.length / 4)
  const gridCount = Math.min(64, totalLeds)

  const grid = []
  for (let i = 0; i < gridCount; i++) grid.push(ledToHex(ledAtIndex(data, i)))

  const underglow = []
  for (let i = 64; i < totalLeds; i++) underglow.push(ledToHex(ledAtIndex(data, i)))

  return {
    timestamp: new Date().toISOString(),
    format: 'rgbw-hex',
    grid,
    underglow,
  }
}

export function getLed(params) {
  const id = params?.id
  if (!id) return { __error: ERR.INVALID_PARAMS }

  const data = getFramebufferData()
  if (!data) return { __error: ERR.UNSUPPORTED }

  const idx = resolveLedIndex(id)
  if (idx === null) return { __error: ERR.UNKNOWN_TARGET }

  if (idx * 4 + 3 >= data.length) return { __error: ERR.UNKNOWN_TARGET }
  return { id, color: ledToHex(ledAtIndex(data, idx)) }
}
