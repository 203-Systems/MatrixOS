function getModule() {
  return window.Module ?? null
}

function withUtf8Pointer(mod, text, callback) {
  if (!mod?._malloc || !mod?._free || !mod?.HEAPU8) return null

  const encoded = new TextEncoder().encode(text)
  const ptr = mod._malloc(encoded.length + 1)
  mod.HEAPU8.set(encoded, ptr)
  mod.HEAPU8[ptr + encoded.length] = 0

  try {
    return callback(ptr, encoded.length)
  } finally {
    mod._free(ptr)
  }
}

function withByteBuffer(mod, bytes, callback) {
  if (!mod?._malloc || !mod?._free || !mod?.HEAPU8) return null

  const ptr = mod._malloc(bytes.length || 1)
  if (bytes.length > 0) {
    mod.HEAPU8.set(bytes, ptr)
  }

  try {
    return callback(ptr, bytes.length)
  } finally {
    mod._free(ptr)
  }
}

export function isFilesystemMounted() {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_FsMounted) return false
  return mod._MatrixOS_Wasm_FsMounted() !== 0
}

export function listFilesystemDirectory(path = '/') {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_FsListJson || !mod?.UTF8ToString) {
    return { ok: false, error: 'Filesystem bridge not available.', path, entries: [] }
  }

  return withUtf8Pointer(mod, path, (pathPtr) => {
    const jsonPtr = mod._MatrixOS_Wasm_FsListJson(pathPtr)
    if (!jsonPtr) {
      return { ok: false, error: 'Filesystem listing returned no data.', path, entries: [] }
    }

    try {
      return JSON.parse(mod.UTF8ToString(jsonPtr))
    } catch (error) {
      return {
        ok: false,
        error: error instanceof Error ? error.message : String(error),
        path,
        entries: [],
      }
    }
  }) ?? { ok: false, error: 'Failed to allocate filesystem listing request.', path, entries: [] }
}

export function readFilesystemFile(path) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_FsReadFile || !mod?._MatrixOS_Wasm_FsReadFileSize || !mod?.HEAPU8) return null

  return withUtf8Pointer(mod, path, (pathPtr) => {
    const dataPtr = mod._MatrixOS_Wasm_FsReadFile(pathPtr)
    const size = mod._MatrixOS_Wasm_FsReadFileSize() >>> 0
    if (!dataPtr && size > 0) return null
    if (size === 0) return new Uint8Array()
    return new Uint8Array(mod.HEAPU8.buffer, dataPtr, size).slice()
  })
}

export function writeFilesystemFile(path, bytes) {
  const mod = getModule()
  const data = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes ?? [])
  if (!mod?._MatrixOS_Wasm_FsWriteFile) return false

  return !!withUtf8Pointer(mod, path, (pathPtr) => (
    withByteBuffer(mod, data, (dataPtr, dataLength) => (
      mod._MatrixOS_Wasm_FsWriteFile(pathPtr, dataPtr, dataLength) !== 0
    ))
  ))
}

export function deleteFilesystemPath(path) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_FsDelete) return false

  return !!withUtf8Pointer(mod, path, (pathPtr) => mod._MatrixOS_Wasm_FsDelete(pathPtr) !== 0)
}

export function makeFilesystemDirectory(path) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_FsMakeDir) return false

  return !!withUtf8Pointer(mod, path, (pathPtr) => mod._MatrixOS_Wasm_FsMakeDir(pathPtr) !== 0)
}
