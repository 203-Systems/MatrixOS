import { get, writable } from 'svelte/store'
import { moduleReady, moduleRef, sendFnKey, sendGridKey, sendKeyInfoEvent, tickKeypad } from './wasm.js'
import { logInputEvent } from './input.js'

const VENDOR_ID_203_SYSTEMS = 0x0203
const MATRIXOS_USAGE_PAGE = 0xFF00
const MATRIXOS_USAGE = 0x01

const SYSTEM_REPORT_ID = 0xCB
const APP_REPORT_ID = 0xFF
const APP_REPORT_SIZE = 63
const APP_COMMAND_SIZE = 1
const PROTOCOL_VERSION = 0x01

const COMMAND_GET_APP_ID = 0x10
const COMMAND_OPEN_DEVELOPER_APP = 0x44

const COMMAND_PING = 0x00
const COMMAND_SET_INPUT_REPORT = 0x01
const COMMAND_EXIT_APP = 0x02
const COMMAND_LED_WRITE_INDEX_RANGE = 0x12
const COMMAND_LED_CANVAS_UPDATE = 0x20

const REPLY_INPUT_EVENT = 0x90

const INPUT_REPORT_KEY_INFO = 0x01
const INPUT_EVENT_KEY_INFO = 0x01
const INPUT_CLUSTER_FUNCTION = 0x00
const INPUT_CLUSTER_PRIMARY_GRID = 0x01
const INPUT_MEMBER_FUNCTION = 0x0000

const LED_FLAG_CANVAS = 0x01
const LED_FLAG_COLOR_MODE_SHIFT = 1
const LED_COLOR_RGB565 = 0x02
const LED_PARTITION_GRID = 0x00
const LED_PARTITION_UNDERGLOW = 0x01
const LED_PARTITION_UNDERGLOW_START = 64
const LED_RANGE_RGB565_HEADER_SIZE = 5
const LED_RANGE_RGB565_MAX_COUNT = Math.floor((APP_REPORT_SIZE - APP_COMMAND_SIZE - LED_RANGE_RGB565_HEADER_SIZE) / 2)

const KEYPAD_STATE_ACTIVATED = 1
const KEYPAD_STATE_PRESSED = 2
const KEYPAD_STATE_HOLD = 3
const KEYPAD_STATE_AFTERTOUCH = 4
const KEYPAD_STATE_RELEASED = 5
const KEYPAD_STATE_NAMES = {
  [KEYPAD_STATE_ACTIVATED]: 'Activated',
  [KEYPAD_STATE_PRESSED]: 'Pressed',
  [KEYPAD_STATE_HOLD]: 'Hold',
  [KEYPAD_STATE_AFTERTOUCH]: 'Aftertouch',
  [KEYPAD_STATE_RELEASED]: 'Released',
}

const MIRROR_INTERVAL_MS = 17
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
let pendingReplies = new Map()
let pendingSystemReplies = new Map()
let mirrorTimer = 0
let keypadTickTimer = 0
let mirrorBusy = false
let lastMirrorFrame = null
let lastMirrorFrameAt = 0
let pendingFullSyncIndexes = null
let activePhysicalGridKeys = new Set()
let activePhysicalFnKey = false
let disconnectHookInstalled = false
let userDisconnected = false

function setState(patch) {
  physicalDeviceState.update((state) => ({ ...state, ...patch }))
}

function getState() {
  return get(physicalDeviceState)
}

function signedByte(value) {
  return value > 127 ? value - 256 : value
}

function to7Bit(value) {
  return Math.max(0, Math.min(127, value >> 9))
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

function makeAppReport(command, payload = []) {
  if (payload.length > APP_REPORT_SIZE - APP_COMMAND_SIZE) {
    throw new Error(`DeveloperApp payload is too long: ${payload.length}`)
  }

  const report = new Uint8Array(APP_REPORT_SIZE)
  report[0] = command
  report.set(payload, APP_COMMAND_SIZE)
  return report
}

function reportError(error) {
  const message = error instanceof Error ? error.message : String(error)
  setState({ error: message, status: 'Error' })
}

function releasePhysicalKeys() {
  if (activePhysicalFnKey) {
    sendFnKey(false)
    logInputEvent('fn', 0, 0, 'Released', 0, 0)
    activePhysicalFnKey = false
  }

  activePhysicalGridKeys.forEach((key) => {
    const [xText, yText] = key.split(',')
    const x = Number(xText)
    const y = Number(yText)
    if (Number.isInteger(x) && Number.isInteger(y)) {
      sendGridKey(x, y, false)
      logInputEvent('grid', x, y, 'Released', 0, 0)
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

function resolvePendingReply(replyCommand, value) {
  const pending = pendingReplies.get(replyCommand)
  if (!pending) return

  window.clearTimeout(pending.timer)
  pendingReplies.delete(replyCommand)
  pending.resolve(value)
}

function rejectPendingReply(replyCommand, error) {
  const pending = pendingReplies.get(replyCommand)
  if (!pending) return

  window.clearTimeout(pending.timer)
  pendingReplies.delete(replyCommand)
  pending.reject(error)
}

async function sendSystemReport(payload) {
  if (!activeDevice?.opened) throw new Error('No physical device is open')
  const source = payload instanceof Uint8Array ? payload : new Uint8Array(payload)
  if (source.length > APP_REPORT_SIZE) {
    throw new Error(`System payload is too long: ${source.length}`)
  }
  const report = new Uint8Array(APP_REPORT_SIZE)
  report.set(source)
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
  const report = makeAppReport(command, payload)

  let replyPromise = null
  if (waitForAck) {
    const replyCommand = command | 0x80
    replyPromise = new Promise((resolve, reject) => {
      const timer = window.setTimeout(() => {
        pendingReplies.delete(replyCommand)
        reject(new Error(`DeveloperApp command 0x${command.toString(16)} timed out`))
      }, timeoutMs)
      pendingReplies.set(replyCommand, { command, resolve, reject, timer })
    })
  }

  await activeDevice.sendReport(APP_REPORT_ID, report)
  return waitForAck ? replyPromise : null
}

function parseAppReply(data) {
  if (data.length === 0) return

  const replyCommand = data[0]
  const payload = data.slice(1)

  if ((replyCommand & 0x80) !== 0 && pendingReplies.has(replyCommand)) {
    const command = replyCommand & 0x7F
    const status = payload[0] ?? 0
    const replyData = payload.slice(1)
    if (status === 0) {
      resolvePendingReply(replyCommand, { command, status, data: replyData })
    } else {
      rejectPendingReply(replyCommand, new Error(`DeveloperApp command 0x${command.toString(16)} failed: 0x${status.toString(16)}`))
    }
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

  if (event.reportId === APP_REPORT_ID) {
    parseAppReply(data)
    return
  }

  if (event.reportId === SYSTEM_REPORT_ID) {
    parseSystemReply(data)
  }
}

function isPressedState(state) {
  return state === KEYPAD_STATE_ACTIVATED
    || state === KEYPAD_STATE_PRESSED
}

function isActiveState(state) {
  return isPressedState(state)
    || state === KEYPAD_STATE_HOLD
    || state === KEYPAD_STATE_AFTERTOUCH
}

function forwardInputEvent(clusterId, memberId, state, pressure, velocity, x, y) {
  if (clusterId === INPUT_CLUSTER_FUNCTION && memberId === INPUT_MEMBER_FUNCTION) {
    if (sendKeyInfoEvent(clusterId, memberId, state, pressure, velocity)) return
  } else if (clusterId === INPUT_CLUSTER_PRIMARY_GRID) {
    const logicalMemberId = y * 8 + x
    if (sendKeyInfoEvent(clusterId, logicalMemberId, state, pressure, velocity)) return
  }

  if (state !== KEYPAD_STATE_RELEASED && !isPressedState(state)) return

  const pressed = state !== KEYPAD_STATE_RELEASED
  if (clusterId === INPUT_CLUSTER_FUNCTION && memberId === INPUT_MEMBER_FUNCTION) {
    sendFnKey(pressed)
  } else if (clusterId === INPUT_CLUSTER_PRIMARY_GRID) {
    sendGridKey(x, y, pressed)
  }
}

function handleInputEvent(payload) {
  if (!getState().inputForwardingEnabled || payload[0] !== INPUT_EVENT_KEY_INFO || payload.length < 11) return

  const clusterId = payload[1]
  const memberId = (payload[2] << 8) | payload[3]
  const state = payload[6]
  const pressure = (payload[7] << 8) | payload[8]
  const velocity = (payload[9] << 8) | payload[10]
  const active = isActiveState(state)
  const stateName = KEYPAD_STATE_NAMES[state] || `State ${state}`
  if (!active && state !== KEYPAD_STATE_RELEASED) return

  if (clusterId === INPUT_CLUSTER_FUNCTION && memberId === INPUT_MEMBER_FUNCTION) {
    if (!isPressedState(state) && state !== KEYPAD_STATE_RELEASED && !activePhysicalFnKey) return

    if (state === KEYPAD_STATE_RELEASED) {
      activePhysicalFnKey = false
    } else if (isPressedState(state)) {
      activePhysicalFnKey = true
    }

    forwardInputEvent(clusterId, memberId, state, pressure, velocity, 0, 0)
    logInputEvent('fn', 0, 0, stateName, to7Bit(velocity), to7Bit(pressure))
    setState({ inputEvents: getState().inputEvents + 1 })
    updateKeypadTickLoop()
    return
  }

  if (clusterId !== INPUT_CLUSTER_PRIMARY_GRID) return

  const x = signedByte(payload[4])
  const y = signedByte(payload[5])
  if (x < 0 || y < 0 || x >= 8 || y >= 8) return

  const key = `${x},${y}`
  if (!isPressedState(state) && state !== KEYPAD_STATE_RELEASED && !activePhysicalGridKeys.has(key)) return

  if (state === KEYPAD_STATE_RELEASED) activePhysicalGridKeys.delete(key)
  else if (isPressedState(state) || activePhysicalGridKeys.has(key)) activePhysicalGridKeys.add(key)

  forwardInputEvent(clusterId, memberId, state, pressure, velocity, x, y)
  logInputEvent('grid', x, y, stateName, to7Bit(velocity), to7Bit(pressure))
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
    if (!pendingFullSyncIndexes) {
      pendingFullSyncIndexes = new Set()
      for (let index = 0; index < ledCount; index++) pendingFullSyncIndexes.add(index)
    }
    pendingFullSyncIndexes.forEach((index) => changed.push(index))
    return changed
  }

  if (pendingFullSyncIndexes) {
    pendingFullSyncIndexes.forEach((index) => changed.push(index))
  }

  for (let index = 0; index < ledCount; index++) {
    if (pendingFullSyncIndexes?.has(index)) continue

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

function updateMirrorFrameCache(frame, indexes) {
  if (!lastMirrorFrame || lastMirrorFrame.length !== frame.length) {
    lastMirrorFrame = new Uint8Array(frame.length)
  }

  indexes.forEach((index) => {
    const base = index * 4
    lastMirrorFrame[base] = frame[base]
    lastMirrorFrame[base + 1] = frame[base + 1]
    lastMirrorFrame[base + 2] = frame[base + 2]
    lastMirrorFrame[base + 3] = frame[base + 3]
    pendingFullSyncIndexes?.delete(index)
  })

  if (pendingFullSyncIndexes?.size === 0) {
    pendingFullSyncIndexes = null
  }
}

function encodeRgb565(r, g, b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
}

function collectChangedLedSections(indexes) {
  const sections = []
  let offset = 0

  while (offset < indexes.length) {
    let startIndex = indexes[offset]
    const partition = startIndex < LED_PARTITION_UNDERGLOW_START ? LED_PARTITION_GRID : LED_PARTITION_UNDERGLOW
    const partitionEnd = partition === LED_PARTITION_GRID ? LED_PARTITION_UNDERGLOW_START : Infinity
    let count = 1
    offset++

    while (offset < indexes.length && indexes[offset] === startIndex + count && indexes[offset] < partitionEnd) {
      count++
      offset++
    }

    while (count > 0) {
      const sectionCount = Math.min(count, LED_RANGE_RGB565_MAX_COUNT)
      sections.push({ partition, startIndex, count: sectionCount })
      startIndex += sectionCount
      count -= sectionCount
    }
  }

  return sections
}

async function uploadRgb565Section(frame, section) {
  const { partition, startIndex, count } = section
  const payload = [
    LED_FLAG_CANVAS | (LED_COLOR_RGB565 << LED_FLAG_COLOR_MODE_SHIFT),
    partition,
    (startIndex >> 8) & 0xFF,
    startIndex & 0xFF,
    count,
  ]

  for (let offset = 0; offset < count; offset++) {
    const base = (startIndex + offset) * 4
    const color = encodeRgb565(frame[base], frame[base + 1], frame[base + 2])
    payload.push((color >> 8) & 0xFF, color & 0xFF)
  }

  await sendDeveloperCommand(COMMAND_LED_WRITE_INDEX_RANGE, payload, { waitForAck: false })
}

async function uploadRgb565Sections(frame, sections) {
  for (const section of sections) {
    await uploadRgb565Section(frame, section)
  }
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
    await uploadRgb565Sections(frame, collectChangedLedSections(changed))
    await sendDeveloperCommand(COMMAND_LED_CANVAS_UPDATE, [], { waitForAck: false })
    updateMirrorFrameCache(frame, changed)
    const now = performance.now()
    const mirrorFps = lastMirrorFrameAt > 0 ? 1000 / Math.max(1, now - lastMirrorFrameAt) : 0
    lastMirrorFrameAt = now
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
  pendingFullSyncIndexes = null
  mirrorTimer = window.setInterval(uploadMirrorFrame, MIRROR_INTERVAL_MS)
  void uploadMirrorFrame()
}

function stopMirrorLoop() {
  if (!mirrorTimer) return

  window.clearInterval(mirrorTimer)
  mirrorTimer = 0
  lastMirrorFrame = null
  pendingFullSyncIndexes = null
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

async function initializeDeveloperSession(status = 'Connecting') {
  stopMirrorLoop()
  releasePhysicalKeys()
  clearPendingReplies(new Error('Developer session reinitializing'))
  setState({ developerReady: false, error: '', status })

  await refreshActiveAppId()
  await enterDeveloperApp()
  await refreshActiveAppId()
  await configureInputReports()
  if (getState().mirrorEnabled) startMirrorLoop()
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
  const devices = await refreshPhysicalDevices()
  if (!userDisconnected && !activeDevice && devices.length > 0) {
    await connectPhysicalDevice({ useAuthorized: true, auto: true })
  }
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

  userDisconnected = false
  setState({ connecting: true, error: '', status: options.useAuthorized ? 'Connecting' : 'Selecting HID device' })
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
      connected: true,
      developerReady: false,
      deviceName: activeDevice.productName || 'Mystrix',
      status: 'Connected',
    })

    await refreshPhysicalDevices()
    await initializeDeveloperSession('Connecting')
    setState({ connecting: false, status: 'Connected' })
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

export async function reconnectPhysicalDevice() {
  if (!activeDevice) {
    await connectPhysicalDevice({ useAuthorized: true })
    return
  }

  userDisconnected = false
  setState({ connecting: true, error: '', connected: true, developerReady: false, status: 'Reconnecting' })
  try {
    if (!activeDevice.opened) {
      await activeDevice.open()
    }

    activeDevice.removeEventListener('inputreport', handleInputReport)
    activeDevice.addEventListener('inputreport', handleInputReport)
    setState({ deviceName: activeDevice.productName || 'Mystrix' })

    await initializeDeveloperSession('Reconnecting')
    setState({ connecting: false, status: 'Connected' })
  } catch (error) {
    stopMirrorLoop()
    releasePhysicalKeys()
    setState({ connecting: false, developerReady: false })
    reportError(error)
  }
}

export async function disconnectPhysicalDevice() {
  userDisconnected = true
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
