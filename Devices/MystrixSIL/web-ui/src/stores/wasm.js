// WASM module lifecycle store for MystrixSIL
import { writable, get } from 'svelte/store'
import { hookMidiTap } from './midi.js'
import { hookHidTap } from './hid.js'
import { hookSerialTap } from './serial.js'

export const moduleRef = writable(null)
export const moduleReady = writable(false)
export const wasmMissing = writable(false)
export const runtimeStatus = writable('Waiting for runtime…')
export const versionLabel = writable('…')
export const buildIdentity = writable('Matrix OS')

let wasmSignature = null
let reloadTimer = 0

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

  return () => {
    if (reloadTimer) clearInterval(reloadTimer)
    mod.onAbort = prevAbort
    if (unhookMidi) unhookMidi()
    if (unhookHid) unhookHid()
    if (unhookSerial) unhookSerial()
  }
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

export function tickKeypad() {
  const mod = get(moduleRef)
  if (mod?._MatrixOS_Wasm_KeypadTick) mod._MatrixOS_Wasm_KeypadTick()
}

export function doReboot() {
  runtimeStatus.set('Rebooting…')
  try { window.localStorage.setItem('matrixos-active-section', '') } catch {}
  const mod = get(moduleRef)
  if (mod?._MatrixOS_Wasm_Reboot) mod._MatrixOS_Wasm_Reboot()
}

export function doBootloader() {
  runtimeStatus.set('Entering bootloader…')
  const mod = get(moduleRef)
  if (mod?._MatrixOS_Wasm_Bootloader) mod._MatrixOS_Wasm_Bootloader()
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
