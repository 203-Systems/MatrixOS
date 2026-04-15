/**
 * Serial handle layer for programmatic send actions.
 */

function getModule() {
  return window.Module ?? null
}

export function sendSerialText(text) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_SerialWrite) return false

  const encoder = new TextEncoder()
  const bytes = encoder.encode(text)
  const ptr = mod._malloc(bytes.length + 1)
  mod.HEAPU8.set(bytes, ptr)
  mod.HEAPU8[ptr + bytes.length] = 0
  mod._MatrixOS_Wasm_SerialWrite(ptr, bytes.length)
  mod._free(ptr)
  return true
}

export function sendSerialHex(hexString) {
  const bytes = hexString.trim().split(/\s+/).map(h => parseInt(h, 16)).filter(n => !isNaN(n))
  const text = String.fromCharCode(...bytes)
  return sendSerialText(text)
}
