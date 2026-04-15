// Forward-looking tooling state for MystrixSim dashboard panels
import { writable, derived } from 'svelte/store'
import { moduleRef, moduleReady, wasmMissing, runtimeStatus, versionLabel } from './wasm.js'
import { logMessages, errorCount, warnCount } from './logs.js'
import { inputEvents, activeGridKeys, fnKeyActive, runtimeGridKeys, runtimeFnActive } from './input.js'
import { openTools } from './tools.js'

export const browserCapabilities = writable({
  detected: false,
  secureContext: false,
  midi: false,
  hid: false,
  serial: false,
  gamepad: false,
  webSocket: false,
})

export function detectBrowserCapabilities() {
  if (typeof window === 'undefined' || typeof navigator === 'undefined') return

  browserCapabilities.set({
    detected: true,
    secureContext: !!window.isSecureContext,
    midi: typeof navigator.requestMIDIAccess === 'function',
    hid: typeof navigator.hid !== 'undefined',
    serial: typeof navigator.serial !== 'undefined',
    gamepad: typeof navigator.getGamepads === 'function',
    webSocket: typeof window.WebSocket === 'function',
  })
}

export function formatBytes(bytes) {
  if (!bytes) return '0 B'
  const units = ['B', 'KB', 'MB', 'GB']
  let value = bytes
  let unit = 0
  while (value >= 1024 && unit < units.length - 1) {
    value /= 1024
    unit++
  }
  return `${value >= 10 || unit === 0 ? value.toFixed(0) : value.toFixed(1)} ${units[unit]}`
}

export const usageSnapshot = derived(
  [
    moduleRef,
    moduleReady,
    wasmMissing,
    runtimeStatus,
    versionLabel,
    logMessages,
    errorCount,
    warnCount,
    inputEvents,
    activeGridKeys,
    fnKeyActive,
    runtimeGridKeys,
    runtimeFnActive,
    openTools,
  ],
  ([
    $moduleRef,
    $moduleReady,
    $wasmMissing,
    $runtimeStatus,
    $versionLabel,
    $logMessages,
    $errorCount,
    $warnCount,
    $inputEvents,
    $activeGridKeys,
    $fnKeyActive,
    $runtimeGridKeys,
    $runtimeFnActive,
    $openTools,
  ]) => ({
    runtimeLive: $moduleReady && !$wasmMissing,
    runtimeStatus: $runtimeStatus,
    versionLabel: $versionLabel,
    logCount: $logMessages.length,
    errorCount: $errorCount,
    warnCount: $warnCount,
    inputEventCount: $inputEvents.length,
    injectedHeldCount: $activeGridKeys.size + ($fnKeyActive ? 1 : 0),
    runtimeHeldCount: $runtimeGridKeys.size + ($runtimeFnActive ? 1 : 0),
    openToolCount: $openTools.length,
    heapBytes: $moduleRef?.HEAPU8?.buffer?.byteLength ?? 0,
    lastLogText: $logMessages.length ? $logMessages[$logMessages.length - 1].text : 'No runtime logs yet.',
    lastInputEvent: $inputEvents.length ? $inputEvents[$inputEvents.length - 1] : null,
  })
)

export const uiInspectorHooks = [
  {
    title: 'Input timeline',
    detail: 'The Input panel already emits ordered, timestamped events that can seed deeper UI interaction tracing.',
    status: 'live',
    label: 'Live',
  },
  {
    title: 'Runtime keypad snapshot',
    detail: 'OS-visible grid and FN state are already mirrored from WASM exports without mutating the device panel contract.',
    status: 'live',
    label: 'Live',
  },
  {
    title: 'Focused UI tree export',
    detail: 'Full UI inspection still needs dedicated runtime metadata for active UI stack, focused component, and modal depth.',
    status: 'planned',
    label: 'Planned',
  },
]

export const scenarioHooks = [
  {
    title: 'Capture seeds',
    detail: 'Logs and input events already share stable timestamps and bounded buffers, which keeps a recorder path clean.',
    status: 'partial',
    label: 'Partial',
  },
  {
    title: 'Replay injector lane',
    detail: 'The existing grid/FN injection helpers can become the first scenario replay sink without changing the layout model.',
    status: 'partial',
    label: 'Partial',
  },
  {
    title: 'Scenario annotations',
    detail: 'Scenario labels, checkpoints, and assertions should stay separate from raw input transport once recording lands.',
    status: 'planned',
    label: 'Planned',
  },
]

export const midiHooks = [
  {
    title: 'MIDI input monitor',
    detail: 'Reserve a protocol-aware lane for note, CC, and clock traffic rather than forcing it through keypad views.',
    status: 'planned',
    label: 'Planned',
  },
  {
    title: 'MIDI output mirror',
    detail: 'Output preview should stay distinct from framebuffer rendering so synth/debug traffic remains inspectable.',
    status: 'planned',
    label: 'Planned',
  },
  {
    title: 'Injection path',
    detail: 'Manual note and controller injection can reuse the existing tool-panel action pattern when the runtime bridge exists.',
    status: 'partial',
    label: 'Ready path',
  },
  {
    title: 'Sysex safety',
    detail: 'Capability gating should be explicit before advanced traffic appears, especially for browser-hosted sessions.',
    status: 'planned',
    label: 'Planned',
  },
]

export const hidHooks = [
  {
    title: 'Local keypad loopback',
    detail: 'The current grid/FN injection path already proves the UI-side dispatch contract for human and automated inputs.',
    status: 'live',
    label: 'Live',
  },
  {
    title: 'Browser HID passthrough',
    detail: 'WebHID can stay optional and isolated from MatrixOS-specific keypad semantics once it is wired in.',
    status: 'planned',
    label: 'Planned',
  },
  {
    title: 'Gamepad mapping lane',
    detail: 'Alternate controller mappings should live beside HID tooling, not inside the device renderer.',
    status: 'planned',
    label: 'Planned',
  },
  {
    title: 'Automation driver',
    detail: 'Synthetic test cases can target the same event sinks as live adapters, which keeps replay and HID aligned.',
    status: 'partial',
    label: 'Partial',
  },
]

export const transportHooks = [
  {
    title: 'Local WASM loopback',
    detail: 'The dashboard already talks directly to Emscripten exports and captures runtime logs inside the browser.',
    status: 'live',
    label: 'Live',
  },
  {
    title: 'Structured SDK bridge',
    detail: 'Future remote control should use an explicit message channel for capability negotiation, state sync, and commands.',
    status: 'planned',
    label: 'Planned',
  },
  {
    title: 'Remote debug socket',
    detail: 'A WebSocket transport should extend the local loopback path, not replace it.',
    status: 'planned',
    label: 'Planned',
  },
]

export const debugHooks = [
  {
    title: 'VS Code / GDB adapter lane',
    detail: 'Debugger control traffic should remain separate from logs so editor integrations can attach cleanly later.',
    status: 'planned',
    label: 'Planned',
  },
  {
    title: 'Execution-state metadata',
    detail: 'Breakpoints, stop reasons, and thread state belong in the transport contract instead of panel-specific stores.',
    status: 'planned',
    label: 'Planned',
  },
]

export const usageHooks = [
  {
    title: 'Allocator telemetry',
    detail: 'Heap and task metrics can drop into this panel cleanly once the runtime exposes them.',
    status: 'planned',
    label: 'Planned',
  },
  {
    title: 'Frame pacing',
    detail: 'The existing render loop gives this panel a natural home for future frame-time and update-cost telemetry.',
    status: 'partial',
    label: 'Partial',
  },
  {
    title: 'Scenario budget tracking',
    detail: 'Recorder/replay memory and timeline pressure should be summarized here instead of inside input or serial panels.',
    status: 'planned',
    label: 'Planned',
  },
]
