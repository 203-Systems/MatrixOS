// Storage (NVS + filesystem) store for MystrixSim dashboard
import JSZip from 'jszip'
import { writable, get } from 'svelte/store'
import { moduleReady } from './wasm.js'
import {
  isFilesystemMounted,
  listFilesystemDirectory,
  readFilesystemFile,
  writeFilesystemFile,
  deleteFilesystemPath,
  makeFilesystemDirectory,
} from '../handles/filesystem.js'

export const nvsEntries = writable([])
export const nvsConnected = writable(false)
export const filesystemMounted = writable(false)
export const filesystemPath = writable('/')
export const filesystemEntries = writable([])
export const filesystemBusy = writable('')
export const filesystemError = writable('')

const FILESYSTEM_MOUNT_RETRY_MS = 250
const FILESYSTEM_MOUNT_RETRY_LIMIT = 20

// Fingerprint = "count:hash0_size0_csum0:..." — detects count changes, size changes,
// AND in-place value rewrites via a content sample checksum (first 16 bytes per entry).
// This covers all practical runtime-side NVS mutations without reading all bytes.
let _lastNvsFingerprint = ''
let _filesystemMountRetryHandle = 0

function normalizeFsPath(path) {
  const text = String(path || '/').trim()
  if (!text || text === '/') return '/'

  const parts = text.split('/').filter(Boolean)
  const normalized = []
  for (const part of parts) {
    if (part === '.') continue
    if (part === '..') {
      normalized.pop()
      continue
    }
    normalized.push(part)
  }

  return normalized.length ? `/${normalized.join('/')}` : '/'
}

export function joinFsPath(basePath, childName) {
  const base = normalizeFsPath(basePath)
  const child = String(childName || '').replace(/^\/+/, '')
  if (!child) return base
  return base === '/' ? `/${child}` : `${base}/${child}`
}

export function parentFsPath(path) {
  const normalized = normalizeFsPath(path)
  if (normalized === '/') return '/'
  const index = normalized.lastIndexOf('/')
  return index <= 0 ? '/' : normalized.slice(0, index)
}

export function basenameFsPath(path) {
  const normalized = normalizeFsPath(path)
  if (normalized === '/') return 'root'
  const segments = normalized.split('/').filter(Boolean)
  return segments[segments.length - 1] || 'root'
}

function computeNvsFingerprint(mod, count) {
  if (count === 0) return '0'
  const hashesPtr = mod._MatrixOS_Wasm_NvsGetHashes()
  const heap32 = new Uint32Array(mod.HEAPU8.buffer)
  const heap8 = mod.HEAPU8
  const parts = [String(count)]
  for (let i = 0; i < Math.min(count, 256); i++) {
    const h = heap32[(hashesPtr >> 2) + i]
    const size = mod._MatrixOS_Wasm_NvsGetSize(h)
    // Sample up to 16 bytes of content for a lightweight change checksum
    let contentSum = 0
    if (size > 0 && mod._MatrixOS_Wasm_NvsGetData) {
      const dataPtr = mod._MatrixOS_Wasm_NvsGetData(h)
      if (dataPtr) {
        const sampleLen = Math.min(size, 16)
        for (let j = 0; j < sampleLen; j++) contentSum += heap8[dataPtr + j]
      }
    }
    parts.push(`${h}_${size}_${contentSum}`)
  }
  return parts.join(':')
}

// Refresh NVS entries from WASM
export function refreshNvs() {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_NvsGetCount) {
    nvsConnected.set(false)
    _lastNvsFingerprint = ''
    return
  }

  nvsConnected.set(true)
  const count = mod._MatrixOS_Wasm_NvsGetCount()

  if (count === 0) {
    _lastNvsFingerprint = '0'
    nvsEntries.set([])
    return
  }

  const hashesPtr = mod._MatrixOS_Wasm_NvsGetHashes()
  const heap32 = new Uint32Array(mod.HEAPU8.buffer)
  const entries = []
  const fpParts = [String(count)]

  for (let i = 0; i < count && i < 256; i++) {
    const hash = heap32[(hashesPtr >> 2) + i]
    const size = mod._MatrixOS_Wasm_NvsGetSize(hash)

    // Content sample for fingerprint (first 16 bytes)
    const dataPtr = mod._MatrixOS_Wasm_NvsGetData(hash)
    let contentSum = 0
    let preview = ''
    let rawBytes = ''

    if (dataPtr && size > 0) {
      const bytes = new Uint8Array(mod.HEAPU8.buffer, dataPtr, Math.min(size, 64))
      contentSum = Array.from(bytes.subarray(0, Math.min(size, 16))).reduce((a, b) => a + b, 0)
      rawBytes = Array.from(bytes).map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join(' ')

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
    fpParts.push(`${hash}_${size}_${contentSum}`)
  }

  _lastNvsFingerprint = fpParts.join(':')
  nvsEntries.set(entries)
}

// Poll NVS for runtime-side writes (called by StoragePanel while mounted).
// Detects count changes, per-entry size changes, AND in-place value rewrites
// via a content sample checksum included in the fingerprint.
export function pollNvs() {
  const mod = window.Module
  if (!mod?._MatrixOS_Wasm_NvsGetCount) return
  const count = mod._MatrixOS_Wasm_NvsGetCount()
  const fp = computeNvsFingerprint(mod, count)
  if (fp !== _lastNvsFingerprint) refreshNvs()
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
  const data = new Uint8Array(mod.HEAPU8.buffer, ptr, size).slice()
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

function setFilesystemFailure(message, path) {
  filesystemMounted.set(false)
  filesystemEntries.set([])
  filesystemError.set(message)
  filesystemPath.set(normalizeFsPath(path))
}

function clearFilesystemMountRetry() {
  if (_filesystemMountRetryHandle) {
    window.clearTimeout(_filesystemMountRetryHandle)
    _filesystemMountRetryHandle = 0
  }
}

function resetFilesystemRuntimeState(path = get(filesystemPath) || '/') {
  clearFilesystemMountRetry()
  filesystemMounted.set(false)
  filesystemEntries.set([])
  filesystemBusy.set('')
  filesystemError.set('')
  filesystemPath.set(normalizeFsPath(path))
}

function scheduleFilesystemMountRetry(path = get(filesystemPath) || '/', attempt = 0) {
  clearFilesystemMountRetry()

  const targetPath = normalizeFsPath(path)
  const tryMount = () => {
    _filesystemMountRetryHandle = 0

    if (!get(moduleReady)) {
      return
    }

    if (refreshFilesystem(targetPath, { quietIfUnmounted: true })) {
      filesystemError.set('')
      return
    }

    if (attempt + 1 >= FILESYSTEM_MOUNT_RETRY_LIMIT) {
      setFilesystemFailure('Filesystem is not mounted.', targetPath)
      return
    }

    _filesystemMountRetryHandle = window.setTimeout(() => {
      scheduleFilesystemMountRetry(targetPath, attempt + 1)
    }, FILESYSTEM_MOUNT_RETRY_MS)
  }

  tryMount()
}

export function refreshFilesystem(path = get(filesystemPath) || '/', options = {}) {
  const { quietIfUnmounted = false } = options
  const targetPath = normalizeFsPath(path)
  if (!isFilesystemMounted()) {
    if (quietIfUnmounted) {
      filesystemMounted.set(false)
      filesystemEntries.set([])
      filesystemPath.set(targetPath)
    } else {
      setFilesystemFailure('Filesystem is not mounted.', targetPath)
    }
    return false
  }

  const result = listFilesystemDirectory(targetPath)
  if (!result?.ok) {
    setFilesystemFailure(result?.error || 'Failed to load filesystem directory.', targetPath)
    return false
  }

  filesystemMounted.set(true)
  filesystemPath.set(normalizeFsPath(result.path))
  filesystemEntries.set(result.entries || [])
  filesystemError.set('')
  clearFilesystemMountRetry()
  return true
}

export function navigateFilesystem(path) {
  return refreshFilesystem(path)
}

export function goUpFilesystem() {
  return refreshFilesystem(parentFsPath(get(filesystemPath)))
}

function downloadBlob(blob, filename) {
  const url = URL.createObjectURL(blob)
  const anchor = document.createElement('a')
  anchor.href = url
  anchor.download = filename
  anchor.click()
  URL.revokeObjectURL(url)
}

function withFilesystemBusy(label, action) {
  filesystemBusy.set(label)
  filesystemError.set('')

  return Promise.resolve()
    .then(action)
    .catch((error) => {
      filesystemError.set(error instanceof Error ? error.message : String(error))
      return false
    })
    .finally(() => {
      filesystemBusy.set('')
    })
}

export function downloadFilesystemFile(entry) {
  const filePath = typeof entry === 'string' ? entry : entry?.path
  const fileName = typeof entry === 'string' ? basenameFsPath(entry) : entry?.name || basenameFsPath(filePath)
  if (!filePath) return Promise.resolve(false)

  return withFilesystemBusy(`download:${fileName}`, async () => {
    const bytes = readFilesystemFile(filePath)
    if (bytes == null) {
      throw new Error(`Unable to read ${fileName}.`)
    }

    downloadBlob(new Blob([bytes], { type: 'application/octet-stream' }), fileName)
    return true
  })
}

export function uploadFilesystemFile(file, directory = get(filesystemPath)) {
  if (!file) return Promise.resolve(false)

  return withFilesystemBusy(`upload:${file.name}`, async () => {
    const bytes = new Uint8Array(await file.arrayBuffer())
    const targetPath = joinFsPath(directory, file.name)
    if (!writeFilesystemFile(targetPath, bytes)) {
      throw new Error(`Failed to upload ${file.name}.`)
    }

    refreshFilesystem(directory)
    return true
  })
}

export function deleteFilesystemEntry(entry) {
  const path = typeof entry === 'string' ? entry : entry?.path
  if (!path) return Promise.resolve(false)

  return withFilesystemBusy(`delete:${path}`, async () => {
    if (!deleteFilesystemPath(path)) {
      throw new Error(`Failed to delete ${path}. Directories must be empty before deletion.`)
    }

    refreshFilesystem(get(filesystemPath))
    return true
  })
}

async function appendDirectoryToArchive(zipFolder, path) {
  const listing = listFilesystemDirectory(path)
  if (!listing?.ok) {
    throw new Error(listing?.error || `Failed to list ${path}`)
  }

  for (const entry of listing.entries || []) {
    if (entry.isDir) {
      const nextFolder = zipFolder.folder(entry.name)
      await appendDirectoryToArchive(nextFolder, entry.path)
    } else {
      const bytes = readFilesystemFile(entry.path)
      if (bytes == null) {
        throw new Error(`Failed to read ${entry.path}`)
      }
      zipFolder.file(entry.name, bytes)
    }
  }
}

export function downloadFilesystemExport(basePath = get(filesystemPath)) {
  const exportPath = normalizeFsPath(basePath)
  return withFilesystemBusy(`export:${exportPath}`, async () => {
    const zip = new JSZip()
    await appendDirectoryToArchive(zip, exportPath)
    const blob = await zip.generateAsync({ type: 'blob' })
    downloadBlob(blob, `${basenameFsPath(exportPath)}-filesystem.zip`)
    return true
  })
}

export function importFilesystemArchive(file, basePath = get(filesystemPath)) {
  if (!file) return Promise.resolve(false)

  const importPath = normalizeFsPath(basePath)
  return withFilesystemBusy(`import:${file.name}`, async () => {
    const zip = await JSZip.loadAsync(file)
    const entries = Object.values(zip.files)
      .filter((entry) => entry.name && !entry.name.startsWith('__MACOSX/'))
      .sort((left, right) => {
        if (left.dir !== right.dir) return left.dir ? -1 : 1
        return left.name.length - right.name.length
      })

    for (const entry of entries) {
      const relativePath = entry.name.replace(/\/+$/, '')
      if (!relativePath) continue

      const targetPath = joinFsPath(importPath, relativePath)
      if (entry.dir) {
        if (!makeFilesystemDirectory(targetPath)) {
          throw new Error(`Failed to create directory ${targetPath}`)
        }
        continue
      }

      const bytes = await entry.async('uint8array')
      if (!writeFilesystemFile(targetPath, bytes)) {
        throw new Error(`Failed to write file ${targetPath}`)
      }
    }

    refreshFilesystem(importPath)
    return true
  })
}

// ---------------------------------------------------------------------------
// NVS hash — MatrixOS StringHash / FNV-1a 32-bit.
// MatrixOS hashes C strings through strlen(str) + 1, so the trailing null byte
// is part of the public hash contract.
// Canonical home: rpc.js and UI both import from here, not from each other.
// ---------------------------------------------------------------------------

export function computeNvsHash(text) {
  let hash = 0x811c9dc5
  const bytes = new TextEncoder().encode(text)
  for (const byte of bytes) {
    hash ^= byte
    hash = (Math.imul(hash, 0x01000193)) >>> 0
  }
  hash ^= 0
  hash = (Math.imul(hash, 0x01000193)) >>> 0
  return hash
}

export function nvsHashHex(text) {
  return '0x' + computeNvsHash(text).toString(16).padStart(8, '0').toUpperCase()
}

function initStorageRuntimeSync() {
  if (typeof window === 'undefined') return

  if (window.__matrixosMystrixSimStorageRuntimeSyncDispose) {
    window.__matrixosMystrixSimStorageRuntimeSyncDispose()
  }

  const unsubscribe = moduleReady.subscribe((ready) => {
    const currentPath = get(filesystemPath) || '/'

    if (!ready) {
      resetFilesystemRuntimeState(currentPath)
      return
    }

    refreshNvs()
    scheduleFilesystemMountRetry(currentPath)
  })

  window.__matrixosMystrixSimStorageRuntimeSyncDispose = () => {
    clearFilesystemMountRetry()
    unsubscribe()
    if (window.__matrixosMystrixSimStorageRuntimeSyncDispose) {
      window.__matrixosMystrixSimStorageRuntimeSyncDispose = null
    }
  }
}

initStorageRuntimeSync()
