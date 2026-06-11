import { get, writable } from 'svelte/store'
import { moduleReady, moduleRef, sendFnKey, sendGridKey, tickKeypad } from './wasm.js'
import { logInputEvent } from './input.js'

const VENDOR_ID_203_SYSTEMS = 0x0203
const MATRIXOS_USAGE_PAGE = 0xFF00
const MATRIXOS_USAGE = 0x01

const SYSTEM_REPORT_ID = 0xCB
const APP_REPORT_ID = 0xFF
const APP_REPORT_SIZE = 32
const APP_HEADER_SIZE = 4
const PROTOCOL_VERSION = 0x01

const COMMAND_GET_APP_ID = 0x10
const COMMAND_OPEN_DEVELOPER_APP = 0x44

const COMMAND_PING = 0x00
const COMMAND_SET_INPUT_REPORT = 0x01
const COMMAND_EXIT_APP = 0x02
const COMMAND_LED_WRITE_INDEX_BULK = 0x18
const COMMAND_LED_CANVAS_UPDATE = 0x20

const REPLY_ACK = 0x80
const REPLY_ERROR = 0x81
const REPLY_INPUT_EVENT = 0x90

const INPUT_REPORT_KEY_INFO = 0x01
const INPUT_EVENT_KEY_INFO = 0x01
const INPUT_CLUSTER_FUNCTION = 0x00
const INPUT_CLUSTER_PRIMARY_GRID = 0x01
const INPUT_MEMBER_FUNCTION = 0x0000

const LED_FLAG_CANVAS = 0x01
const LED_FLAG_RGBW = 0x02
const LED_BULK_RGBW_ENTRY_SIZE = 6
const LED_BULK_RGBW_MAX_COUNT = Math.floor((APP_REPORT_SIZE - APP_HEADER_SIZE - 2) / LED_BULK_RGBW_ENTRY_SIZE)

const KEYPAD_STATE_ACTIVATED = 1
const KEYPAD_STATE_PRESSED = 2
const KEYPAD_STATE_HOLD = 3
const KEYPAD_STATE_AFTERTOUCH = 4
const KEYPAD_STATE_RELEASED = 5

const MIRROR_INTERVAL_MS = 83
const KEYPAD_TICK_INTERVAL_MS = 16
const DEVELOPER_APP_START_DELAY_MS = 250

const initialState = {
  supported: false,
  initialized: false,
  scanning: false,
  connecting: false,
  connected: false,
  developerReady: false,
  deviceName: '',
  status: 'Idle',
  error: '',
  authorizedDevices: [],
  activeAppId: null,
  deviceWidth: 0,
  deviceHeight: 0,
  ledCount: 0,
  mirrorEnabled: true,
  inputForwardingEnabled: true,
  mirrorFps: 0,
  mirroredFrames: 0,
  mirroredLeds: 0,
  inputEvents: 0,
}

export const physicalDeviceState = writable({ ...initialState })

let activeDevice = null
let seq = 1
let pendingReplies = new Map()
let pendingSystemReplies = new Map()
let mirrorTimer = 0
let keypadTickTimer = 0
let mirrorBusy = false
let lastMirrorFrame = null
let lastMirrorFrameAt = 0
let activePhysicalGridKeys = new Set()
let activePhysicalFnKey = false
let disconnectHookInstalled = false

function setState(patch) {
  physicalDeviceState.update((state) => ({ ...state, ...patch }))
}

function getState() {
  return get(physicalDeviceState)
}

function nextSeq() {
  const value = seq
  seq = seq >= 255 ? 1 : seq + 1
  return value
}

function signedByte(value) {
  return value > 127 ? value - 256 : value
}

function delay(ms) {
  return new Promise((resolve) => window.setTimeout(resolve, ms))
}

function isWebHidAvailable() {
  return typeof navigator !== 'undefined' && typeof navigator.hid !== 'undefined'
}

function deviceSummary(device) {
  return {
    productName: device.productName || 'HID Device',
    vendorId: device.vendorId ?? 0,
    productId: device.productId ?? 0,
    opened: Boolean(device.opened),
  }
}

function hasMatrixOSCollection(device) {
  return (device.collections || []).some((collection) => {
    return collection.usagePage === MATRIXOS_USAGE_PAGE && collection.usage === MATRIXOS_USAGE
  })
}

function sortPhysicalDevices(devices) {
  return [...devices].sort((left, right) => {
    return Number(hasMatrixOSCollection(right)) - Number(hasMatrixOSCollection(left))
  })
}

function makeAppReport(command, payload = [], reportSeq = nextSeq()) {
  if (payload.length > APP_REPORT_SIZE - APP_HEADER_SIZE) {
    throw new Error(`DeveloperApp payload is too long: ${payload.length}`)
  }

  const report = new Uint8Array(APP_REPORT_SIZE)
  report[0] = PROTOCOL_VERSION
  report[1] = reportSeq
  report[2] = command
  report[3] = payload.length
  report.set(payload, APP_HEADER_SIZE)
  return { report, reportSeq }
}

function reportError(error) {
  const message = error instanceof Error ? error.message : String(error)
  setState({ error: message, status: 'Error' })
}

function releasePhysicalKeys() {
  if (activePhysicalFnKey) {
    sendFnKey(false)
    logInputEvent('fn', 0, 0, false)
    activePhysicalFnKey = false
  }

  activePhysicalGridKeys.forEach((key) => {
    const [xText, yText] = key.split(',')
    const x = Number(xText)
    const y = Number(yText)
    if (Number.isInteger(x) && Number.isInteger(y)) {
      sendGridKey(x, y, false)
      logInputEvent('grid', x, y, false)
    }
  })
  activePhysicalGridKeys.clear()
  stopKeypadTickLoop()
}

function clearPendingReplies(error = new Error('Device disconnected')) {
  pendingReplies.forEach(({ reject, timer }) => {
    window.clearTimeout(timer)
    reject(error)
  })
  pendingReplies.clear()

  pendingSystemReplies.forEach(({ reject, timer }) => {
    window.clearTimeout(timer)
    reject(error)
  })
  pendingSystemReplies.clear()
}

function resolvePendingReply(reportSeq, value) {
  const pending = pendingReplies.get(reportSeq)
  if (!pending) return

  window.clearTimeout(pending.timer)
  pendingReplies.delete(reportSeq)
  pending.resolve(value)
}

function rejectPendingReply(reportSeq, error) {
  const pending = pendingReplies.get(reportSeq)
  if (!pending) return

  window.clearTimeout(pending.timer)
  pendingReplies.delete(reportSeq)
  pending.reject(error)
}

async function sendSystemReport(payload) {
  if (!activeDevice?.opened) throw new Error('No physical device is open')
  const report = payload instanceof Uint8Array ? payload : new Uint8Array(payload)
  if (report.length > APP_REPORT_SIZE) {
    throw new Error(`System payload is too long: ${report.length}`)
  }
  await activeDevice.sendReport(SYSTEM_REPORT_ID, report)
}

async function requestSystemReport(command, payload = [], timeoutMs = 600) {
  if (!activeDevice?.opened) throw new Error('No physical device is open')

  const replyCommand = command | 0x80
  const replyPromise = new Promise((resolve, reject) => {
    const timer = window.setTimeout(() => {
      pendingSystemReplies.delete(replyCommand)
      reject(new Error(`System command 0x${command.toString(16)} timed out`))
    }, timeoutMs)
    pendingSystemReplies.set(replyCommand, { resolve, reject, timer })
  })

  await sendSystemReport(new Uint8Array([command, ...payload]))
  return replyPromise
}

async function sendDeveloperCommand(command, payload = [], options = {}) {
  if (!activeDevice?.opened) throw new Error('No physical device is open')

  const { waitForAck = true, timeoutMs = 800 } = options
  const { report, reportSeq } = makeAppReport(command, payload)

  let replyPromise = null
  if (waitForAck) {
    replyPromise = new Promise((resolve, reject) => {
      const timer = window.setTimeout(() => {
        pendingReplies.delete(reportSeq)
        reject(new Error(`DeveloperApp command 0x${command.toString(16)} timed out`))
      }, timeoutMs)
      pendingReplies.set(reportSeq, { command, resolve, reject, timer })
    })
  }

  await activeDevice.sendReport(APP_REPORT_ID, report)
  return waitForAck ? replyPromise : null
}

function parseAppReply(data) {
  if (data.length < APP_HEADER_SIZE || data[0] !== PROTOCOL_VERSION) return

  const reportSeq = data[1]
  const replyCommand = data[2]
  const length = Math.min(data[3], data.length - APP_HEADER_SIZE)
  const payload = data.slice(APP_HEADER_SIZE, APP_HEADER_SIZE + length)

  if (replyCommand === REPLY_ACK) {
    const command = payload[0] ?? 0
    const status = payload[1] ?? 0
    const replyData = payload.slice(2)
    resolvePendingReply(reportSeq, { command, status, data: replyData })
    return
  }

  if (replyCommand === REPLY_ERROR) {
    const command = payload[0] ?? 0
    const errorCode = payload[1] ?? 0
    rejectPendingReply(reportSeq, new Error(`DeveloperApp command 0x${command.toString(16)} failed: 0x${errorCode.toString(16)}`))
    return
  }

  if (replyCommand === REPLY_INPUT_EVENT) {
    handleInputEvent(payload)
  }
}

function parseSystemReply(data) {
  const replyCommand = data[0]
  const pending = pendingSystemReplies.get(replyCommand)
  if (pending) {
    window.clearTimeout(pending.timer)
    pendingSystemReplies.delete(replyCommand)
    pending.resolve(data)
  }

  if (replyCommand !== (COMMAND_GET_APP_ID | 0x80) || data.length < 5) return

  const appId = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4]
  setState({ activeAppId: appId >>> 0 })
}

function handleInputReport(event) {
  const data = new Uint8Array(event.data.buffer, event.data.byteOffset, event.data.byteLength)

  if (event.reportId === APP_REPORT_ID && data[0] === PROTOCOL_VERSION) {
    parseAppReply(data)
    return
  }

  if (event.reportId === SYSTEM_REPORT_ID || event.reportId === APP_REPORT_ID) {
    parseSystemReply(data)
  }
}

function isPressedState(state) {
  return state === KEYPAD_STATE_ACTIVATED
    || state === KEYPAD_STATE_PRESSED
    || state === KEYPAD_STATE_HOLD
    || state === KEYPAD_STATE_AFTERTOUCH
}

function handleInputEvent(payload) {
  if (!getState().inputForwardingEnabled || payload[0] !== INPUT_EVENT_KEY_INFO || payload.length < 11) return

  const clusterId = payload[1]
  const memberId = (payload[2] << 8) | payload[3]
  const state = payload[6]
  const pressed = isPressedState(state)
  if (!pressed && state !== KEYPAD_STATE_RELEASED) return

  if (clusterId === INPUT_CLUSTER_FUNCTION && memberId === INPUT_MEMBER_FUNCTION) {
    if (pressed === activePhysicalFnKey) return

    activePhysicalFnKey = pressed
    sendFnKey(pressed)
    logInputEvent('fn', 0, 0, pressed)
    setState({ inputEvents: getState().inputEvents + 1 })
    updateKeypadTickLoop()
    return
  }

  if (clusterId !== INPUT_CLUSTER_PRIMARY_GRID) return

  const x = signedByte(payload[4])
  const y = signedByte(payload[5])
  if (x < 0 || y < 0 || x >= 8 || y >= 8) return

  const key = `${x},${y}`
  const wasPressed = activePhysicalGridKeys.has(key)
  if (pressed === wasPressed) return

  if (pressed) activePhysicalGridKeys.add(key)
  else activePhysicalGridKeys.delete(key)

  sendGridKey(x, y, pressed)
  logInputEvent('grid', x, y, pressed)
  setState({ inputEvents: getState().inputEvents + 1 })
  updateKeypadTickLoop()
}

function readFrameBuffer() {
  if (!get(moduleReady)) return null

  const mod = get(moduleRef)
  const heap = mod?.HEAPU8 || window.HEAPU8
  const ptr = mod?._MatrixOS_Wasm_GetFrameBuffer?.() ?? 0
  const byteLength = mod?._MatrixOS_Wasm_GetFrameBufferByteLength?.() ?? 0
  if (!heap || !ptr || !byteLength) return null

  return new Uint8Array(heap.slice(ptr, ptr + byteLength))
}

function collectChangedLedIndexes(frame) {
  const ledCount = Math.floor(frame.length / 4)
  const changed = []

  if (!lastMirrorFrame || lastMirrorFrame.length !== frame.length) {
    for (let index = 0; index < ledCount; index++) changed.push(index)
    return changed
  }

  for (let index = 0; index < ledCount; index++) {
    const base = index * 4
    if (
      frame[base] !== lastMirrorFrame[base]
      || frame[base + 1] !== lastMirrorFrame[base + 1]
      || frame[base + 2] !== lastMirrorFrame[base + 2]
      || frame[base + 3] !== lastMirrorFrame[base + 3]
    ) {
      changed.push(index)
    }
  }

  return changed
}

async function uploadLedChunk(frame, indexes) {
  const payload = [
    LED_FLAG_CANVAS | LED_FLAG_RGBW,
    indexes.length,
  ]

  indexes.forEach((index) => {
    const base = index * 4
    payload.push((index >> 8) & 0xFF, index & 0xFF, frame[base], frame[base + 1], frame[base + 2], frame[base + 3])
  })

  await sendDeveloperCommand(COMMAND_LED_WRITE_INDEX_BULK, payload, { waitForAck: false })
}

async function uploadMirrorFrame() {
  const state = getState()
  if (!state.connected || !state.developerReady || !state.mirrorEnabled || mirrorBusy) return

  const frame = readFrameBuffer()
  if (!frame) return

  const changed = collectChangedLedIndexes(frame)
  if (changed.length === 0) return

  mirrorBusy = true
  try {
    for (let offset = 0; offset < changed.length; offset += LED_BULK_RGBW_MAX_COUNT) {
      await uploadLedChunk(frame, changed.slice(offset, offset + LED_BULK_RGBW_MAX_COUNT))
    }
    await sendDeveloperCommand(COMMAND_LED_CANVAS_UPDATE, [], { waitForAck: false })
    const now = performance.now()
    const mirrorFps = lastMirrorFrameAt > 0 ? 1000 / Math.max(1, now - lastMirrorFrameAt) : 0
    lastMirrorFrameAt = now
    lastMirrorFrame = frame
    setState({
      mirroredFrames: state.mirroredFrames + 1,
      mirroredLeds: state.mirroredLeds + changed.length,
      mirrorFps,
      error: '',
      status: 'Live',
    })
  } catch (error) {
    reportError(error)
  } finally {
    mirrorBusy = false
  }
}

function startMirrorLoop() {
  if (mirrorTimer) return

  lastMirrorFrame = null
  mirrorTimer = window.setInterval(uploadMirrorFrame, MIRROR_INTERVAL_MS)
  void uploadMirrorFrame()
}

function stopMirrorLoop() {
  if (!mirrorTimer) return

  window.clearInterval(mirrorTimer)
  mirrorTimer = 0
  lastMirrorFrame = null
  lastMirrorFrameAt = 0
}

function startKeypadTickLoop() {
  if (keypadTickTimer) return

  keypadTickTimer = window.setInterval(() => {
    if (!activePhysicalFnKey && activePhysicalGridKeys.size === 0) {
      stopKeypadTickLoop()
      return
    }
    tickKeypad()
  }, KEYPAD_TICK_INTERVAL_MS)
}

function stopKeypadTickLoop() {
  if (!keypadTickTimer) return

  window.clearInterval(keypadTickTimer)
  keypadTickTimer = 0
}

function updateKeypadTickLoop() {
  if (activePhysicalFnKey || activePhysicalGridKeys.size > 0) startKeypadTickLoop()
  else stopKeypadTickLoop()
}

async function pingDeveloperApp() {
  const reply = await sendDeveloperCommand(COMMAND_PING, [], { timeoutMs: 800 })
  const data = reply.data
  if (data.length < 5 || data[0] !== PROTOCOL_VERSION) {
    throw new Error('DeveloperApp ping returned an invalid reply')
  }

  const ledCount = (data[3] << 8) | data[4]
  setState({
    developerReady: true,
    deviceWidth: data[1],
    deviceHeight: data[2],
    ledCount,
    status: 'Connected',
    error: '',
  })
}

async function enterDeveloperApp() {
  setState({ status: 'Connecting', developerReady: false })
  await requestSystemReport(COMMAND_OPEN_DEVELOPER_APP, [], 800)
  await delay(DEVELOPER_APP_START_DELAY_MS)

  let lastError = null
  for (let attempt = 0; attempt < 15; attempt++) {
    try {
      await pingDeveloperApp()
      return
    } catch (error) {
      lastError = error
      await delay(150)
    }
  }

  const detail = lastError instanceof Error ? lastError.message : String(lastError || '')
  throw new Error(`Device did not enter DeveloperApp after command 0x44${detail ? `: ${detail}` : ''}`)
}

async function refreshActiveAppId() {
  try {
    const data = await requestSystemReport(COMMAND_GET_APP_ID)
    if (data.length < 5) return null
    return ((data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4]) >>> 0
  } catch {
    return null
  }
}

async function configureInputReports() {
  if (!getState().developerReady) return
  await sendDeveloperCommand(COMMAND_SET_INPUT_REPORT, [
    INPUT_REPORT_KEY_INFO,
    getState().inputForwardingEnabled ? 1 : 0,
  ])
}

function handleDisconnect(event) {
  if (event.device !== activeDevice) return
  clearPendingReplies()
  releasePhysicalKeys()
  stopMirrorLoop()
  activeDevice = null
  setState({
    connected: false,
    developerReady: false,
    deviceName: '',
    activeAppId: null,
    status: 'Disconnected',
  })
}

function installDisconnectHook() {
  if (disconnectHookInstalled || !isWebHidAvailable()) return

  navigator.hid.addEventListener('disconnect', handleDisconnect)
  disconnectHookInstalled = true
}

export async function initializePhysicalBridge() {
  const supported = isWebHidAvailable()
  setState({ supported, initialized: true })
  if (!supported) return

  installDisconnectHook()
  await refreshPhysicalDevices()
}

export async function refreshPhysicalDevices() {
  if (!isWebHidAvailable()) {
    setState({ supported: false, authorizedDevices: [] })
    return []
  }

  setState({ scanning: true })
  try {
    const devices = await navigator.hid.getDevices()
    const mystrixDevices = sortPhysicalDevices(devices.filter((device) => device.vendorId === VENDOR_ID_203_SYSTEMS && hasMatrixOSCollection(device)))
    const summaries = mystrixDevices.map(deviceSummary)
    setState({ authorizedDevices: summaries, scanning: false, supported: true })
    return mystrixDevices
  } catch (error) {
    setState({ scanning: false })
    reportError(error)
    return []
  }
}

export function getPhysicalDeviceSnapshot() {
  return {
    ...getState(),
    opened: Boolean(activeDevice?.opened),
  }
}

export async function listPhysicalHidDevices() {
  const devices = await refreshPhysicalDevices()
  return devices.map(deviceSummary)
}

export async function connectPhysicalDevice(options = {}) {
  if (!isWebHidAvailable()) {
    setState({ supported: false, status: 'WebHID unavailable' })
    return
  }

  setState({ connecting: true, error: '', status: 'Selecting HID device' })
  try {
    const devices = options.useAuthorized
      ? await refreshPhysicalDevices()
      : await navigator.hid.requestDevice({
        filters: [{ vendorId: VENDOR_ID_203_SYSTEMS, usagePage: MATRIXOS_USAGE_PAGE, usage: MATRIXOS_USAGE }],
      })

    if (devices.length === 0) {
      setState({ connecting: false, status: 'No device selected' })
      return
    }

    const selectedDevice = sortPhysicalDevices(devices)[0]
    releasePhysicalKeys()
    if (activeDevice && activeDevice !== selectedDevice) {
      activeDevice.removeEventListener('inputreport', handleInputReport)
      if (activeDevice.opened) {
        await activeDevice.close()
      }
    }

    activeDevice = selectedDevice
    if (!activeDevice.opened) {
      await activeDevice.open()
    }

    activeDevice.removeEventListener('inputreport', handleInputReport)
    activeDevice.addEventListener('inputreport', handleInputReport)
    clearPendingReplies(new Error('Replaced by a new physical device'))

    setState({
      connecting: false,
      connected: true,
      developerReady: false,
      deviceName: activeDevice.productName || 'Mystrix',
      status: 'Connected',
    })

    await refreshPhysicalDevices()
    await refreshActiveAppId()
    await enterDeveloperApp()
    await refreshActiveAppId()
    await configureInputReports()
    if (getState().mirrorEnabled) startMirrorLoop()
  } catch (error) {
    try {
      if (activeDevice?.opened) {
        activeDevice.removeEventListener('inputreport', handleInputReport)
        await activeDevice.close()
      }
    } catch {}
    activeDevice = null
    stopMirrorLoop()
    releasePhysicalKeys()
    setState({ connecting: false, connected: false, developerReady: false, deviceName: '' })
    reportError(error)
  }
}

export async function disconnectPhysicalDevice() {
  stopMirrorLoop()
  clearPendingReplies()
  releasePhysicalKeys()

  try {
    if (activeDevice?.opened) {
      await exitDeveloperApp()
      activeDevice.removeEventListener('inputreport', handleInputReport)
      await activeDevice.close()
    }
  } catch (error) {
    reportError(error)
  } finally {
    activeDevice = null
    setState({
      connected: false,
      developerReady: false,
      deviceName: '',
      activeAppId: null,
      status: 'Disconnected',
    })
  }
}

async function exitDeveloperApp() {
  if (!getState().developerReady) return

  try {
    await sendDeveloperCommand(COMMAND_EXIT_APP, [], { timeoutMs: 500 })
  } catch {}
}

export function disposePhysicalBridge() {
  stopMirrorLoop()
  releasePhysicalKeys()
  clearPendingReplies()
  if (activeDevice?.opened) {
    activeDevice.removeEventListener('inputreport', handleInputReport)
    void exitDeveloperApp()
    void activeDevice.close().catch(() => {})
  }
  activeDevice = null
  if (disconnectHookInstalled && isWebHidAvailable()) {
    navigator.hid.removeEventListener('disconnect', handleDisconnect)
    disconnectHookInstalled = false
  }
}
