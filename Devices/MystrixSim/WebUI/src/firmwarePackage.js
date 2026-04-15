import JSZip from 'jszip'

export const MSPKG_SUFFIX = '.mspkg'

function normalizeBytes(input) {
  if (input instanceof Uint8Array) return input
  if (input instanceof ArrayBuffer) return new Uint8Array(input)
  return new Uint8Array(input || [])
}

export async function extractMspkgPackage(input, label = 'MatrixOS.mspkg') {
  const zip = await JSZip.loadAsync(input)
  const files = Object.values(zip.files).filter((entry) => !entry.dir)
  const jsEntry = files.find((entry) => entry.name.split('/').pop() === 'MatrixOSHost.js')
  const wasmEntry = files.find((entry) => entry.name.split('/').pop() === 'MatrixOSHost.wasm')

  if (!jsEntry || !wasmEntry) {
    throw new Error(`Package ${label} is missing MatrixOSHost.js or MatrixOSHost.wasm`)
  }

  const [jsText, wasmBytes] = await Promise.all([
    jsEntry.async('string'),
    wasmEntry.async('uint8array'),
  ])

  return { jsText, wasmBytes }
}

export async function sha256Hex(input) {
  const bytes = normalizeBytes(input)

  if (!globalThis.crypto?.subtle) {
    return `${bytes.byteLength.toString(16).padStart(8, '0')}-nocrypto`
  }

  const digest = await globalThis.crypto.subtle.digest('SHA-256', bytes)
  return Array.from(new Uint8Array(digest), (value) => value.toString(16).padStart(2, '0')).join('')
}

export function toArrayBuffer(input) {
  const bytes = normalizeBytes(input)
  return bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength)
}
