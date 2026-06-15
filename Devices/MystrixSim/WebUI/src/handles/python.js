import { isActiveApp } from './application.js'

function getModule() {
  return window.Module ?? null
}

function isRuntimeReady(mod) {
  return !!mod && (mod.runtimeReady || mod.calledRun)
}

function isEmscriptenUnwind(error) {
  return error === 'unwind' || error?.message === 'unwind'
}

function withUtf8Pointer(mod, text, callback) {
  if (!mod?._malloc || !mod?._free || !mod?.HEAPU8) return false

  const encoder = new TextEncoder()
  const bytes = encoder.encode(text)
  const ptr = mod._malloc(bytes.length + 1)
  mod.HEAPU8.set(bytes, ptr)
  mod.HEAPU8[ptr + bytes.length] = 0

  try {
    return callback(ptr, bytes.length)
  } finally {
    mod._free(ptr)
  }
}

export function hasPythonApp() {
  const mod = getModule()
  if (!isRuntimeReady(mod)) return false
  if (!mod?._MatrixOS_Wasm_HasPythonApp) return false
  try {
    return mod._MatrixOS_Wasm_HasPythonApp() !== 0
  } catch (error) {
    if (isEmscriptenUnwind(error)) return true
    throw error
  }
}

export function isPythonAppActive() {
  const mod = getModule()
  if (!isRuntimeReady(mod)) return false
  if (mod?._MatrixOS_Wasm_IsPythonAppActive) {
    try {
      return mod._MatrixOS_Wasm_IsPythonAppActive() !== 0
    } catch (error) {
      if (isEmscriptenUnwind(error)) return true
      throw error
    }
  }
  return isActiveApp('203 Systems', 'Python')
}

export function getPythonSessionMode() {
  const mod = getModule()
  if (!isRuntimeReady(mod)) return 'none'
  if (!mod?._MatrixOS_Wasm_GetPythonSessionMode) {
    return isPythonAppActive() ? 'repl' : 'none'
  }

  let mode
  try {
    mode = mod._MatrixOS_Wasm_GetPythonSessionMode()
  } catch (error) {
    if (isEmscriptenUnwind(error)) return 'unknown'
    throw error
  }

  switch (mode) {
    case 1: return 'repl'
    case 2: return 'app'
    case 0: return 'none'
    default: return 'unknown'
  }
}

export function getPythonDebugInfo() {
  const mod = getModule()
  if (!isRuntimeReady(mod)) {
    return {
      available: false,
      active: false,
      mode: 'none',
      runtime: { initialized: false, heapSize: 0 },
    }
  }

  const base = {
    available: hasPythonApp(),
    active: isPythonAppActive(),
    mode: getPythonSessionMode(),
  }
  const scripts = getPythonStagedFilesInfo(mod)

  if (!mod?._MatrixOS_Wasm_GetPythonDebugJson || !mod?.UTF8ToString) {
    return {
      ...base,
      supported: false,
      scripts,
      runtime: { initialized: false, heapSize: 0 },
    }
  }

  try {
    const ptr = mod._MatrixOS_Wasm_GetPythonDebugJson()
    const parsed = ptr ? JSON.parse(mod.UTF8ToString(ptr)) : {}
    return {
      ...base,
      supported: true,
      scripts,
      ...parsed,
    }
  } catch (error) {
    if (isEmscriptenUnwind(error)) {
      return {
        ...base,
        supported: true,
        active: true,
        mode: base.mode === 'none' ? 'unknown' : base.mode,
        scripts,
        runtime: { initialized: true, heapSize: 0 },
      }
    }
    return {
      ...base,
      supported: true,
      scripts,
      error: String(error?.message || error),
      runtime: { initialized: false, heapSize: 0 },
    }
  }
}

function getPythonStagedFilesInfo(mod) {
  if (!mod?._MatrixOS_Wasm_GetPythonStagedFilesJson || !mod?.UTF8ToString) {
    return {
      supported: false,
      entry: '',
      count: 0,
      files: [],
    }
  }

  try {
    const ptr = mod._MatrixOS_Wasm_GetPythonStagedFilesJson()
    const parsed = ptr ? JSON.parse(mod.UTF8ToString(ptr)) : {}
    return {
      supported: true,
      entry: typeof parsed.entry === 'string' ? parsed.entry : '',
      count: Number.isFinite(parsed.count) ? parsed.count : (Array.isArray(parsed.files) ? parsed.files.length : 0),
      files: Array.isArray(parsed.files) ? parsed.files : [],
    }
  } catch (error) {
    return {
      supported: true,
      error: String(error?.message || error),
      entry: '',
      count: 0,
      files: [],
    }
  }
}

export function enterPythonRepl() {
  const mod = getModule()
  if (!isRuntimeReady(mod)) return false
  if (!mod?._MatrixOS_Wasm_PythonEnterRepl) return false
  try {
    return mod._MatrixOS_Wasm_PythonEnterRepl() !== 0
  } catch (error) {
    if (isEmscriptenUnwind(error)) return true
    throw error
  }
}

export function stagePythonScript(fileName, text) {
  const mod = getModule()
  if (!isRuntimeReady(mod)) return false
  if (!mod?._MatrixOS_Wasm_PythonStageScript) return false

  return withUtf8Pointer(mod, fileName, (namePtr) => (
    withUtf8Pointer(mod, text, (textPtr, textLength) => (
      mod._MatrixOS_Wasm_PythonStageScript(namePtr, textPtr, textLength) !== 0
    ))
  ))
}

export function clearStagedPythonScripts() {
  const mod = getModule()
  if (!isRuntimeReady(mod)) return false
  if (!mod?._MatrixOS_Wasm_PythonClearStaged) return false
  mod._MatrixOS_Wasm_PythonClearStaged()
  return true
}

export function stagePythonFiles(files) {
  if (!Array.isArray(files) || files.length === 0) return false
  if (!clearStagedPythonScripts()) return false
  for (const file of files) {
    if (!file || typeof file.name !== 'string' || typeof file.text !== 'string') return false
    if (!stagePythonScript(file.name, file.text)) return false
  }
  return true
}

export function runStagedPythonScript() {
  const mod = getModule()
  if (!isRuntimeReady(mod)) return false
  if (!mod?._MatrixOS_Wasm_PythonRunStaged) return false
  try {
    return mod._MatrixOS_Wasm_PythonRunStaged() !== 0
  } catch (error) {
    if (isEmscriptenUnwind(error)) return true
    throw error
  }
}

export function stopPythonApp() {
  const mod = getModule()
  if (!isRuntimeReady(mod)) return false
  if (!mod?._MatrixOS_Wasm_PythonStop) return false
  try {
    return mod._MatrixOS_Wasm_PythonStop() !== 0
  } catch (error) {
    if (isEmscriptenUnwind(error)) return true
    throw error
  }
}

export function sendPythonInput(text) {
  const mod = getModule()
  if (!isRuntimeReady(mod)) return false
  if (!mod?._MatrixOS_Wasm_PythonInjectInput) return false

  return withUtf8Pointer(mod, text, (textPtr, textLength) => (
    mod._MatrixOS_Wasm_PythonInjectInput(textPtr, textLength) !== 0
  ))
}
