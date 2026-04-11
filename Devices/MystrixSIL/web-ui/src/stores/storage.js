// Storage (NVS + filesystem) store for MystrixSIL dashboard
import { writable, get } from 'svelte/store'

export const nvsEntries = writable([])
export const nvsConnected = writable(false)
export const filesystemMounted = writable(false)
export const filesystemPath = writable('')

let _lastNvsCount = -1

// Refresh NVS entries from WASM
export function refreshNvs() {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_NvsGetCount) {
    nvsConnected.set(false)
    _lastNvsCount = -1
    return
  }

  nvsConnected.set(true)
  const count = mod._MatrixOS_Wasm_NvsGetCount()

  if (count === 0) {
    _lastNvsCount = 0
    nvsEntries.set([])
    return
  }

  const hashesPtr = mod._MatrixOS_Wasm_NvsGetHashes()
  const heap32 = new Uint32Array(mod.HEAPU8.buffer)
  const entries = []

  for (let i = 0; i < count && i < 256; i++) {
    const hash = heap32[(hashesPtr >> 2) + i]
    const size = mod._MatrixOS_Wasm_NvsGetSize(hash)
    const dataPtr = mod._MatrixOS_Wasm_NvsGetData(hash)
    let preview = ''
    let rawBytes = ''

    if (dataPtr && size > 0) {
      const bytes = new Uint8Array(mod.HEAPU8.buffer, dataPtr, Math.min(size, 64))
      rawBytes = Array.from(bytes).map(b => b.toString(16).padStart(2, '0').toUpperCase()).join(' ')

      // Try to decode as UTF-8 string
      try {
        const text = new TextDecoder().decode(bytes)
        if (/^[\x20-\x7E]*$/.test(text) && text.length > 0) {
          preview = `"${text}"`
        }
      } catch {}

      // Try integer interpretation for small sizes
      if (!preview) {
        if (size === 1) preview = `${bytes[0]} (u8)`
        else if (size === 2) preview = `${bytes[0] | (bytes[1] << 8)} (u16)`
        else if (size === 4) {
          const view = new DataView(bytes.buffer, bytes.byteOffset, 4)
          preview = `${view.getUint32(0, true)} (u32)`
        }
      }

      if (!preview) preview = `${size} bytes`
    }

    entries.push({
      hash,
      hashHex: '0x' + hash.toString(16).padStart(8, '0').toUpperCase(),
      size,
      preview,
      rawBytes,
    })
  }

  _lastNvsCount = count
  nvsEntries.set(entries)
}

// Poll NVS for runtime-side writes (called by StoragePanel while mounted)
export function pollNvs() {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_NvsGetCount) return
  const count = mod._MatrixOS_Wasm_NvsGetCount()
  if (count !== _lastNvsCount) refreshNvs()
}

// Write an NVS entry
export function writeNvsEntry(hash, data) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_NvsWrite || !mod.HEAPU8) return false
  const ptr = mod._malloc(data.length)
  mod.HEAPU8.set(data, ptr)
  const result = mod._MatrixOS_Wasm_NvsWrite(hash, ptr, data.length)
  mod._free(ptr)
  if (result) refreshNvs()
  return !!result
}

// Delete an NVS entry
export function deleteNvsEntry(hash) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_NvsDelete) return false
  const result = mod._MatrixOS_Wasm_NvsDelete(hash)
  if (result) refreshNvs()
  return !!result
}

// Clear all NVS entries
export function clearNvs() {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_NvsClear) return
  mod._MatrixOS_Wasm_NvsClear()
  refreshNvs()
}

// Export NVS to a downloadable blob
export function exportNvs() {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_NvsExport || !mod?._MatrixOS_Wasm_NvsExportSize) return null
  const ptr = mod._MatrixOS_Wasm_NvsExport()
  const size = mod._MatrixOS_Wasm_NvsExportSize()
  if (!ptr || !size) return null
  const data = new Uint8Array(mod.HEAPU8.buffer, ptr, size).slice() // copy
  return data
}

// Import NVS from a blob
export function importNvs(data) {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_NvsImport || !mod.HEAPU8) return false
  const ptr = mod._malloc(data.length)
  mod.HEAPU8.set(data, ptr)
  mod._MatrixOS_Wasm_NvsImport(ptr, data.length)
  mod._free(ptr)
  refreshNvs()
  return true
}

// Download NVS export as file
export function downloadNvsExport() {
  const data = exportNvs()
  if (!data) return false
  const blob = new Blob([data], { type: 'application/octet-stream' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = 'matrixos-nvs-export.bin'
  a.click()
  URL.revokeObjectURL(url)
  return true
}

// Import NVS from file input
export function importNvsFromFile(file) {
  return new Promise((resolve) => {
    const reader = new FileReader()
    reader.onload = (e) => {
      const data = new Uint8Array(e.target.result)
      resolve(importNvs(data))
    }
    reader.onerror = () => resolve(false)
    reader.readAsArrayBuffer(file)
  })
}