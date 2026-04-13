/**
 * USB handle layer for programmatic availability toggles.
 */

export function getUsbAvailable() {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_GetUsbAvailable) return null
  return mod._MatrixOS_Wasm_GetUsbAvailable() !== 0
}

export function setUsbAvailable(available) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_SetUsbAvailable) return false
  mod._MatrixOS_Wasm_SetUsbAvailable(available ? 1 : 0)
  return true
}

