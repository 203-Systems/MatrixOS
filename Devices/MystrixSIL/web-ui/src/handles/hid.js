/**
 * HID handle layer for programmatic send actions.
 */

export function sendRawHid(bytes) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_RawHidInject || !mod.HEAPU8) return false

  const payload = bytes instanceof Uint8Array ? bytes : Uint8Array.from(bytes ?? [])
  const len = Math.min(payload.length, 32)
  const ptr = mod._malloc(len)
  mod.HEAPU8.set(payload.slice(0, len), ptr)
  mod._MatrixOS_Wasm_RawHidInject(ptr, len)
  mod._free(ptr)
  return true
}
