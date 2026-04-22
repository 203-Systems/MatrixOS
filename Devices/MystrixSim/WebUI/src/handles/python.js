import { isActiveApp } from './application.js'

function getModule() {
  return window.Module ?? null
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
  return !!mod?._MatrixOS_Wasm_HasPythonApp && mod._MatrixOS_Wasm_HasPythonApp() !== 0
}

export function isPythonAppActive() {
  const mod = getModule()
  if (mod?._MatrixOS_Wasm_IsPythonAppActive) {
    return mod._MatrixOS_Wasm_IsPythonAppActive() !== 0
  }
  return isActiveApp('203 Systems', 'Python')
}

export function getPythonSessionMode() {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_GetPythonSessionMode) {
    return isPythonAppActive() ? 'repl' : 'none'
  }

  switch (mod._MatrixOS_Wasm_GetPythonSessionMode()) {
    case 1: return 'repl'
    case 2: return 'app'
    case 0: return 'none'
    default: return 'unknown'
  }
}

export function enterPythonRepl() {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_PythonEnterRepl) return false
  return mod._MatrixOS_Wasm_PythonEnterRepl() !== 0
}

export function stagePythonScript(fileName, text) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_PythonStageScript) return false

  return withUtf8Pointer(mod, fileName, (namePtr) => (
    withUtf8Pointer(mod, text, (textPtr, textLength) => (
      mod._MatrixOS_Wasm_PythonStageScript(namePtr, textPtr, textLength) !== 0
    ))
  ))
}

export function runStagedPythonScript() {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_PythonRunStaged) return false
  return mod._MatrixOS_Wasm_PythonRunStaged() !== 0
}

export function sendPythonInput(text) {
  const mod = getModule()
  if (!mod?._MatrixOS_Wasm_PythonInjectInput) return false

  return withUtf8Pointer(mod, text, (textPtr, textLength) => (
    mod._MatrixOS_Wasm_PythonInjectInput(textPtr, textLength) !== 0
  ))
}
