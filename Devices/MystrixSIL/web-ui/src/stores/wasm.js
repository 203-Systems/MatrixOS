// WASM module lifecycle store for MystrixSIL
import { writable, get } from 'svelte/store'
import { hookMidiTap, clearMidiEvents } from './midi.js'
import { hookHidTap, clearHidEvents } from './hid.js'
import { hookSerialTap, clearSerialEvents } from './serial.js'
import { clearLogs } from './logs.js'
import { clearInputEvents } from './input.js'

export const moduleRef = writable(null)
export const moduleReady = writable(false)
export const wasmMissing = writable(false)
export const runtimeStatus = writable('Waiting for runtime…')
export const versionLabel = writable('…')
export const buildIdentity = writable('Matrix OS')

let wasmSignature = null
let reloadTimer = 0
let _currentCleanup = null
let _restartInProgress = false
// Blob URL for the current injected MatrixOSHost.js — revoked on next teardown.
let _currentBlobUrl = null

const isHtmlResponse = (r) => (r.headers.get('content-type') || '').includes('text/html')

export async function checkWasmAvailability() {
  try {
    const r = await fetch('/MatrixOSHost.wasm', { method: 'HEAD', cache: 'no-store' })
    if ((!r.ok && r.status === 404) || (r.ok && isHtmlResponse(r))) {
      wasmMissing.set(true)
      runtimeStatus.set('WASM image missing')
      return false
    }
    if (r.ok) wasmMissing.set(false)
    return r.ok
  } catch {
    return false
  }
}

async function checkWasmUpdate() {
  try {
    const r = await fetch('/MatrixOSHost.wasm', { method: 'HEAD', cache: 'no-store' })
    if (!r.ok) {
      if (r.status === 404) {
        wasmMissing.set(true)
        runtimeStatus.set('WASM image missing')
      }
      return
    }
    if (isHtmlResponse(r)) {
      wasmMissing.set(true)
      runtimeStatus.set('WASM image missing')
      return
    }
    wasmMissing.set(false)
    const sig = r.headers.get('etag') || r.headers.get('last-modified') || r.headers.get('content-length')
    if (!sig) return
    if (wasmSignature && sig !== wasmSignature) {
      console.info('MatrixOS wasm update detected, reloading.')
      window.location.reload()
      return
    }
    wasmSignature = sig
  } catch {}
}

// ---------------------------------------------------------------------------
// Hot-restart lifecycle
// ---------------------------------------------------------------------------

const looksLikeMatrixOSLog = (args) => {
  const text = args.map(v => {
    if (v === null || v === undefined) return String(v)
    if (typeof v === 'object') { try { return JSON.stringify(v) } catch { return '[object]' } }
    return String(v)
  }).join(' ').trim()
  return text.startsWith('[MystrixSIL]') || /^[DIWEV]\s*\(\d+\)/.test(text)
}

/** Terminate the current WASM module and all its pthreads. */
function terminateWasmRuntime() {
  const mod = window.Module
  if (!mod) return

  try {
    if (mod.PThread) {
      if (typeof mod.PThread.terminateAllThreads === 'function') {
        mod.PThread.terminateAllThreads()
      } else {
        ;(mod.PThread.runningWorkers || []).forEach(w => { try { w.terminate() } catch {} })
        ;(mod.PThread.unusedWorkers || []).forEach(w => { try { w.terminate() } catch {} })
      }
    }
  } catch (e) {
    console.warn('[MystrixSIL] Error terminating pthreads:', e)
  }

  // Revoke the previous restart's Blob URL now that all workers are terminated.
  if (_currentBlobUrl) {
    try { URL.revokeObjectURL(_currentBlobUrl) } catch {}
    _currentBlobUrl = null
  }

  document.querySelectorAll('script[src*="MatrixOSHost.js"]').forEach(el => el.remove())
  // Emscripten may define HEAPU8 as non-configurable — use assignment, not delete
  try { window.HEAPU8 = undefined } catch {}
  try { window.Module = undefined } catch {}
}

/** Set up a fresh window.Module with all required callbacks. */
function prepareFreshModule() {
  window.MatrixOSLogBuffer = []

  window.MatrixOSRuntimeReady = new Promise((resolve) => {
    window.Module = {
      runtimeReady: false,
      onRuntimeInitialized() {
        this.runtimeReady = true
        resolve()
      },
      // When MatrixOSHost.js is executed via new Function() (hot-restart path),
      // document.currentScript is null, so Emscripten sets _scriptName=undefined.
      // This causes pthread workers to be created at URL 'undefined' → HTTP 404/HTML.
      // mainScriptUrlOrBlob overrides the worker URL — Emscripten checks this first.
      mainScriptUrlOrBlob: `${location.origin}/MatrixOSHost.js`,
      locateFile: (path) => path ? `/${path}` : '/',
      print: (...args) => {
        console.log(...args)
        if (window.MatrixOSLogDispatch) window.MatrixOSLogDispatch('log', args)
        else window.MatrixOSLogBuffer.push({ level: 'log', args })
      },
      printErr: (...args) => {
        console.error(...args)
        if (window.MatrixOSLogDispatch && looksLikeMatrixOSLog(args)) {
          window.MatrixOSLogDispatch('error', args)
        } else if (looksLikeMatrixOSLog(args)) {
          window.MatrixOSLogBuffer.push({ level: 'error', args })
        }
      },
    }
  })

  window.MatrixOS_Reboot = () => { setTimeout(restartWasm, 0) }
  // Bootloader is distinct from a hot-restart: in the SIL environment there is no
  // actual DFU target, so entering bootloader is represented as a full page reload
  // (restoring the original pre-hot-restart behavior). This keeps it clearly
  // separate from the runtime-only restart path.
  window.MatrixOS_Bootloader = () => { location.reload() }
}

/** Dynamically inject MatrixOSHost.js and wait for it to load. */
async function injectWasmScript() {
  // Emscripten uses let/const at top level of its JS output — injecting a
  // second <script> tag into the page causes "already declared" SyntaxErrors.
  //
  // Instead, fetch the JS text and run it via new Function() so each restart
  // gets an isolated lexical scope.
  //
  // Three problems to solve inside new Function():
  //
  // 1. Module scope: new Function() has no outer Module. Emscripten's
  //    `var Module = typeof Module != 'undefined' ? Module : {}`
  //    creates a fresh empty {}, ignoring our window.Module callbacks.
  //    Fix: preamble sets `var Module = window.Module || {}`.
  //
  // 2. Pthread worker URL: Emscripten derives the worker script URL from
  //    `document.currentScript.src`, which is null inside new Function().
  //    Workers end up created at URL `undefined` → HTTP 404.
  //    Fix: create a Blob URL from our preamble+script, set it as
  //    Module.mainScriptUrlOrBlob so Emscripten uses it for workers.
  //
  // 3. locateFile in workers: pthread workers run in a scope without
  //    window.Module (window is undefined in workers). We add a fallback
  //    locateFile in the preamble for the worker context.
  const url = `/MatrixOSHost.js?t=${Date.now()}`
  let text
  try {
    const r = await fetch(url, { cache: 'no-store' })
    if (!r.ok) throw new Error(`HTTP ${r.status}`)
    text = await r.text()
  } catch (e) {
    wasmMissing.set(true)
    runtimeStatus.set('WASM image missing')
    throw new Error(`Failed to fetch MatrixOSHost.js: ${e.message}`)
  }

  // Preamble that runs in BOTH main thread and pthread worker contexts:
  //   - Main thread: `window` is defined → Module = window.Module (our callbacks)
  //   - Worker: `window` is undefined → Module = {} (worker takes its own init path)
  //   - locateFile fallback ensures workers can find MatrixOSHost.wasm
  const preamble = [
    `var Module = (typeof window !== 'undefined' ? window.Module : null) || {};`,
    `if (!Module.locateFile) Module.locateFile = function(path) { return '/' + path; };`,
  ].join('\n') + '\n'

  const wrapped = preamble + text

  // Create a Blob URL from the full wrapped script. Emscripten's pthread support
  // uses Module.mainScriptUrlOrBlob as the worker URL — this ensures workers load
  // the same preamble+script from the blob (not from document.currentScript which
  // is null in new Function() context).
  let blobUrl = null
  try {
    const blob = new Blob([wrapped], { type: 'application/javascript' })
    blobUrl = URL.createObjectURL(blob)
    if (window.Module) window.Module.mainScriptUrlOrBlob = blobUrl
  } catch {
    // Blob URLs may be unavailable in some environments — fall back gracefully.
    if (window.Module) window.Module.mainScriptUrlOrBlob = `${location.origin}/MatrixOSHost.js`
  }

  try {
    // eslint-disable-next-line no-new-func
    new Function(wrapped)()
  } catch (e) {
    if (blobUrl) { try { URL.revokeObjectURL(blobUrl) } catch {} }
    wasmMissing.set(true)
    runtimeStatus.set('WASM image missing')
    throw new Error(`Failed to execute MatrixOSHost.js: ${e.message}`)
  }
  // Store the blob URL at module level. It must remain alive while any pthread
  // worker holds a reference to it. terminateWasmRuntime() will revoke it on
  // the next hot-restart once all workers are terminated.
  _currentBlobUrl = blobUrl
}

/** Reset all runtime-facing event stores. */
function resetRuntimeStores() {
  clearLogs()
  clearMidiEvents()
  clearHidEvents()
  clearSerialEvents()
  clearInputEvents()
}

/**
 * Hot-restart: tear down the current WASM runtime and start a fresh one.
 * The Svelte UI shell stays mounted throughout.
 */
export async function restartWasm() {
  if (_restartInProgress) return
  _restartInProgress = true
  console.info('[MystrixSIL] Runtime hot-restart initiated')

  try {
    // Phase 1: Teardown
    moduleReady.set(false)
    moduleRef.set(null)
    runtimeStatus.set('Restarting…')
    versionLabel.set('…')
    buildIdentity.set('Matrix OS')
    wasmMissing.set(false)
    wasmSignature = null

    if (_currentCleanup) { _currentCleanup(); _currentCleanup = null }
    terminateWasmRuntime()
    resetRuntimeStores()

    // Brief pause to let workers drain
    await new Promise(r => setTimeout(r, 150))

    // Phase 2: Fresh load
    prepareFreshModule()
    await injectWasmScript()

    // Phase 3: Re-init
    initWasm()

    console.info('[MystrixSIL] Runtime hot-restart complete — polling for readiness')
  } catch (e) {
    console.error('[MystrixSIL] Hot-restart failed:', e)
    runtimeStatus.set('Restart failed')
  } finally {
    _restartInProgress = false
  }
}

// Expose for console debugging
window.matrixosRestart = restartWasm

export function initWasm() {
  checkWasmAvailability()

  const mod = window.Module ?? null
  moduleRef.set(mod)

  if (!mod) {
    if (!get(wasmMissing)) runtimeStatus.set('WASM not loaded')
    return () => {}
  }

  // Install subsystem tap hooks so events arrive before runtime starts
  const unhookMidi = hookMidiTap()
  const unhookHid = hookHidTap()
  const unhookSerial = hookSerialTap()

  // Hook abort
  const prevAbort = mod.onAbort
  mod.onAbort = (what) => {
    if (typeof prevAbort === 'function') prevAbort(what)
    wasmMissing.set(true)
    runtimeStatus.set('WASM image missing')
  }

  // Hot-reload polling
  checkWasmUpdate()
  reloadTimer = window.setInterval(checkWasmUpdate, 2000)

  // Wait for runtime then poll for readiness
  const waitForRuntime = () => {
    const ready = window.MatrixOSRuntimeReady
    if (ready && typeof ready.then === 'function') return ready
    return new Promise((resolve) => {
      if (!mod) { resolve(); return }
      if (mod.runtimeReady) { resolve(); return }
      const prev = mod.onRuntimeInitialized
      mod.onRuntimeInitialized = () => {
        if (typeof prev === 'function') prev()
        mod.runtimeReady = true
        resolve()
      }
    })
  }

  const startPoll = () => {
    if (get(wasmMissing)) { setTimeout(startPoll, 250); return }
    if (!mod.runtimeReady && !mod.calledRun) { setTimeout(startPoll, 50); return }
    const heap = mod.HEAPU8 || window.HEAPU8
    if (!heap || !mod._MatrixOS_Wasm_GetFrameBuffer) {
      runtimeStatus.set('Waiting for framebuffer…')
      setTimeout(startPoll, 50)
      return
    }
    moduleReady.set(true)
    runtimeStatus.set('Live')
    // Ensure USB starts connected so boot animation auto-progresses
    if (mod._MatrixOS_Wasm_SetUsbAvailable) mod._MatrixOS_Wasm_SetUsbAvailable(1)
    if (mod._MatrixOS_Wasm_GetVersionString && mod.UTF8ToString) {
      const ptr = mod._MatrixOS_Wasm_GetVersionString()
      if (ptr) versionLabel.set(mod.UTF8ToString(ptr))
    }
    if (mod._MatrixOS_Wasm_GetBuildIdentityString && mod.UTF8ToString) {
      const ptr = mod._MatrixOS_Wasm_GetBuildIdentityString()
      if (ptr) buildIdentity.set(mod.UTF8ToString(ptr))
    }
  }

  waitForRuntime().then(startPoll)

  // Upgrade reboot hook to use hot-restart (setTimeout avoids EM_ASM deadlock).
  // Bootloader uses location.reload() — there is no DFU target in the SIL
  // environment, so bootloader semantics are a full page reload, not a hot-restart.
  window.MatrixOS_Reboot = () => { setTimeout(restartWasm, 0) }
  window.MatrixOS_Bootloader = () => { location.reload() }

  const cleanup = () => {
    if (reloadTimer) clearInterval(reloadTimer)
    mod.onAbort = prevAbort
    if (unhookMidi) unhookMidi()
    if (unhookHid) unhookHid()
    if (unhookSerial) unhookSerial()
  }
  _currentCleanup = cleanup
  return cleanup
}

export function sendGridKey(x, y, pressed) {
  const mod = get(moduleRef)
  if (!mod?._MatrixOS_Wasm_KeyEvent) return
  mod._MatrixOS_Wasm_KeyEvent(x, y, pressed ? 1 : 0)
}

export function sendFnKey(pressed) {
  const mod = get(moduleRef)
  if (!mod?._MatrixOS_Wasm_FnEvent) return
  mod._MatrixOS_Wasm_FnEvent(pressed ? 1 : 0)
}

// side: 0 = left touchbar, 1 = right touchbar; index: 0..7
export function sendTouchBarKey(side, index, pressed) {
  const mod = get(moduleRef)
  if (!mod?._MatrixOS_Wasm_TouchBarEvent) return
  mod._MatrixOS_Wasm_TouchBarEvent(side, index, pressed ? 1 : 0)
}

export function tickKeypad() {
  const mod = get(moduleRef)
  if (mod?._MatrixOS_Wasm_KeypadTick) mod._MatrixOS_Wasm_KeypadTick()
}

export function doReboot() {
  restartWasm()
}

export function doBootloader() {
  // Bootloader is a distinct action from a hot-restart. In the SIL environment
  // there is no physical DFU target, so entering bootloader reloads the full page
  // (matching the original pre-hot-restart behavior).
  location.reload()
}

export function getRotation() {
  const mod = get(moduleRef)
  if (mod?._MatrixOS_Wasm_GetRotation) return mod._MatrixOS_Wasm_GetRotation()
  return 0
}

export function getUptimeMs() {
  const mod = get(moduleRef)
  if (mod?._MatrixOS_Wasm_GetUptimeMs) return mod._MatrixOS_Wasm_GetUptimeMs()
  return 0
}

// Runtime-side keypad state (polled by Input panel)

export function getRuntimeKeypadState() {
  const mod = get(moduleRef)
  if (!mod?._MatrixOS_Wasm_GetKeypadState || !mod?._MatrixOS_Wasm_GetKeypadStateLength) return null
  const heap = mod.HEAPU8 || window.HEAPU8
  if (!heap) return null
  const len = mod._MatrixOS_Wasm_GetKeypadStateLength()
  const ptr = mod._MatrixOS_Wasm_GetKeypadState()
  if (!ptr || !len) return null
  return heap.subarray(ptr, ptr + len)
}

export function getRuntimeFnState() {
  const mod = get(moduleRef)
  if (!mod?._MatrixOS_Wasm_GetFnState) return false
  return mod._MatrixOS_Wasm_GetFnState() !== 0
}

// Accept HMR updates for this module so Vite re-evaluates wasm.js in place
// (rather than propagating to parent Svelte components and losing context).
// window.matrixosRestart, doReboot, initWasm, etc. are all updated on re-eval.
if (import.meta.hot) {
  // Clean up the old module's side effects before the new module takes over.
  // Must mirror the teardown performed by terminateWasmRuntime() + the cleanup
  // closure, because HMR re-evaluation replaces this module's closure state.
  import.meta.hot.dispose(() => {
    // Stop the WASM-update polling timer.
    if (reloadTimer) { clearInterval(reloadTimer); reloadTimer = 0 }
    // Run the active subsystem-hook cleanup (MIDI/HID/Serial unhook, onAbort restore).
    if (_currentCleanup) { _currentCleanup(); _currentCleanup = null }
    // Terminate any running pthread workers so they don't linger after the
    // new module version takes over.
    terminateWasmRuntime()
    // Clear the in-progress guard so the new module instance can restart freely.
    _restartInProgress = false
  })
  // After dispose() the old module may have fully torn down window.Module.
  // In that case, explicitly hot-restart the runtime so wasm.js edits recover
  // to a live state without requiring a manual page reload.
  import.meta.hot.accept(() => {
    const mod = window.Module
    if (mod?.runtimeReady || mod?.calledRun) {
      initWasm()
    } else {
      restartWasm()
    }
  })
}
