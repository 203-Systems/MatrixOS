/**
 * Session / runtime handle layer.
 *
 * Owns the JSON-RPC session metadata and runtime state/query responses for:
 *   - session.status
 *   - session.ping
 *   - session.reset
 *   - runtime.getState
 *   - runtime.getAppState
 */

import { get } from 'svelte/store'
import {
  moduleReady, runtimeStatus, versionLabel, buildIdentity,
  doReboot, getUptimeMs,
} from '../stores/wasm.js'
import { errorCount, warnCount } from '../stores/logs.js'
import { getUsbAvailable } from './usb.js'

export function getSessionStatus({ sessionId, protocolVersion }) {
  return {
    protocolVersion,
    sessionId,
    connected: true,
    runtimeReady: get(moduleReady),
    build: (get(buildIdentity) || get(versionLabel) || 'Matrix OS').replace(/\s*•\s*/g, ' ').trim(),
  }
}

export function pingSession() {
  return { ok: true }
}

export function resetSession() {
  doReboot()
  return { ok: true }
}

export function getRuntimeState() {
  const ready = get(moduleReady)
  const usbConnected = getUsbAvailable()
  return {
    status: get(runtimeStatus),
    uptimeMs: getUptimeMs(),
    usbConnected: usbConnected == null ? false : usbConnected,
    activeApp: ready ? { name: 'Running', id: 'runtime' } : null,
    // Use MatrixOS log store counts — do not mix in emulator-host JS errors
    warningCount: get(warnCount),
    errorCount: get(errorCount),
  }
}

export function getRuntimeAppState() {
  return {
    activeApp: get(moduleReady) ? { name: 'Running', id: 'runtime' } : null,
  }
}
