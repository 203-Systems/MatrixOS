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

  document.querySelectorAll('script[src*="MatrixOSHost.js"]').forEach(el => el.remove())
  delete window.HEAPU8
  window.Module = undefined
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
      locateFile: (path) => `/${path}`,
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
  window.MatrixOS_Bootloader = () => { setTimeout(restartWasm, 0) }
}

/** Dynamically inject MatrixOSHost.js and wait for it to load. */
function injectWasmScript() {
  return new Promise((resolve, reject) => {
    const script = document.createElement('script')
    script.src = `/MatrixOSHost.js?t=${Date.now()}`
    script.onload = resolve
    script.onerror = () => {
      wasmMissing.set(true)
      runtimeStatus.set('WASM image missing')
      reject(new Error('Failed to load MatrixOSHost.js'))
    }
    document.body.appendChild(script)
  })
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

  // Upgrade reboot hooks to use hot-restart (setTimeout avoids EM_ASM deadlock)
  window.MatrixOS_Reboot = () => { setTimeout(restartWasm, 0) }
  window.MatrixOS_Bootloader = () => { setTimeout(restartWasm, 0) }

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
  restartWasm()
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
