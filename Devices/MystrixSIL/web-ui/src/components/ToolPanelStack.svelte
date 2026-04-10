<script>
  import { onMount, tick } from 'svelte'
  import { Close } from 'carbon-icons-svelte'
  import { openTools, closeTool, deviceTools } from '../stores/tools.js'
  import InputPanel from './InputPanel.svelte'
  import APIPanel from './tools/APIPanel.svelte'
  import LogsPanel from './LogsPanel.svelte'
  import RuntimePanel from './RuntimePanel.svelte'
  import ApplicationPanel from './tools/ApplicationPanel.svelte'
  import UIPanel from './tools/UIPanel.svelte'
  import MIDIPanel from './tools/MIDIPanel.svelte'
  import HIDPanel from './tools/HIDPanel.svelte'
  import SerialPanel from './tools/SerialPanel.svelte'
  import UsagePanel from './tools/UsagePanel.svelte'
  import USBPanel from './tools/USBPanel.svelte'
  import GyroPanel from './tools/GyroPanel.svelte'
  import BatteryPanel from './tools/BatteryPanel.svelte'
  import StoragePanel from './tools/StoragePanel.svelte'

  const panelMap = {
    application: ApplicationPanel,
    api: APIPanel,
    input: InputPanel,
    logs: LogsPanel,
    runtime: RuntimePanel,
    ui: UIPanel,
    midi: MIDIPanel,
    hid: HIDPanel,
    serial: SerialPanel,
    usage: UsagePanel,
    usb: USBPanel,
    gyro: GyroPanel,
    battery: BatteryPanel,
    storage: StoragePanel,
  }

  /* ── Layout constants (must match CSS / other components) ── */
  const LEFT_NAV_W = 60
  const TRAY_W     = 48
  const MIN_CENTER = 420
  const MIN_COL    = 220
  const DIVIDER_W  = 6

  /* Default percentage of available width by tool count */
  const DEFAULT_PCT = { 1: 0.30, 2: 0.50, 3: 0.60 }
  const DEFAULT_PCT_MAX = 0.68

  /* ── Persisted keys ── */
  const widthPrefKey      = 'matrixos-tool-stack-width'
  const panelWidthPrefKey = 'matrixos-tool-panel-widths'

  /* ── State ── */
  let stackWidth      = null
  let userOverride     = null   // null → use default; number → user-dragged px
  let panelWidths      = {}     // { toolId: fraction }
  let resizingStack    = false
  let activeDividerIndex = -1
  let panelStack
  let lastToolSignature = ''
  let prevToolCount     = 0

  /* ── Helpers ── */
  function getLabel(id) {
    const tool = deviceTools.find(t => t.id === id)
    return tool ? tool.label : id
  }

  function available() {
    if (typeof window === 'undefined') return 800
    return window.innerWidth - LEFT_NAV_W - TRAY_W
  }

  function defaultWidth(toolCount) {
    const pct = DEFAULT_PCT[toolCount] ?? DEFAULT_PCT_MAX
    return Math.round(available() * pct)
  }

  function clampWidth(px, toolCount) {
    const avail       = available()
    const minForTools = toolCount * MIN_COL + Math.max(toolCount - 1, 0) * DIVIDER_W
    const minW        = Math.max(minForTools, MIN_COL)
    const maxW        = Math.max(avail - MIN_CENTER, minW)
    return Math.max(minW, Math.min(maxW, Math.round(px)))
  }

  function resolveWidth(toolCount) {
    const baseWidth = clampWidth(defaultWidth(toolCount), toolCount)
    if (userOverride == null) return baseWidth
    return Math.max(baseWidth, clampWidth(userOverride, toolCount))
  }

  /* ── Panel proportions ── */
  function normalizePanelWidths(toolIds) {
    const next = {}
    let total = 0
    for (const id of toolIds) {
      const v = panelWidths[id]
      const safe = typeof v === 'number' && Number.isFinite(v) && v > 0 ? v : 1
      next[id] = safe
      total += safe
    }
    if (total <= 0) {
      for (const id of toolIds) next[id] = 1 / Math.max(toolIds.length, 1)
      return next
    }
    for (const id of toolIds) next[id] /= total
    return next
  }

  function persistPanelWidths() {
    try { window.localStorage.setItem(panelWidthPrefKey, JSON.stringify(panelWidths)) } catch {}
  }

  function persistOverride() {
    try {
      if (userOverride != null) window.localStorage.setItem(widthPrefKey, String(userOverride))
      else window.localStorage.removeItem(widthPrefKey)
    } catch {}
  }

  async function syncLayoutForTools(toolIds) {
    const sig = toolIds.join('|')
    if (sig === lastToolSignature) return
    lastToolSignature = sig
    panelWidths = normalizePanelWidths(toolIds)
    persistPanelWidths()
    prevToolCount = toolIds.length
    await tick()
    if (toolIds.length > 0) {
      stackWidth = resolveWidth(toolIds.length)
    }
  }

  function getPanelStyle(toolId) {
    const w = panelWidths[toolId] ?? 0
    const dividers = Math.max($openTools.length - 1, 0) * DIVIDER_W
    return `width:calc((100% - ${dividers}px) * ${w});flex:0 0 auto;`
  }

  /* ── Left-boundary drag (sets userOverride) ── */
  function startResize(event) {
    event.preventDefault()
    resizingStack = true
    const onMove = (e) => {
      const newW = window.innerWidth - TRAY_W - e.clientX
      const count = $openTools.length
      userOverride = clampWidth(newW, count)
      stackWidth = userOverride
      persistOverride()
    }
    const onUp = () => {
      resizingStack = false
      window.removeEventListener('pointermove', onMove)
      window.removeEventListener('pointerup', onUp)
    }
    window.addEventListener('pointermove', onMove)
    window.addEventListener('pointerup', onUp)
  }

  /* ── Inter-column divider drag (proportions only) ── */
  function startPanelResize(index, event) {
    event.preventDefault()
    const toolIds = [...$openTools]
    const leftId  = toolIds[index]
    const rightId = toolIds[index + 1]
    if (!leftId || !rightId || !panelStack) return

    activeDividerIndex = index
    const rect          = panelStack.getBoundingClientRect()
    const dividerCount  = Math.max(toolIds.length - 1, 0)
    const totalContentW = rect.width - dividerCount * DIVIDER_W
    const normalized    = normalizePanelWidths(toolIds)
    const pairTotal     = (normalized[leftId] + normalized[rightId]) * totalContentW
    const startX        = event.clientX
    const startLeftPx   = normalized[leftId] * totalContentW

    const onMove = (e) => {
      const delta      = e.clientX - startX
      const nextLeftPx  = Math.max(MIN_COL, Math.min(pairTotal - MIN_COL, startLeftPx + delta))
      const nextRightPx = pairTotal - nextLeftPx
      panelWidths = {
        ...normalized,
        [leftId]:  nextLeftPx  / totalContentW,
        [rightId]: nextRightPx / totalContentW,
      }
      persistPanelWidths()
    }

    const onUp = () => {
      activeDividerIndex = -1
      panelWidths = normalizePanelWidths(toolIds)
      persistPanelWidths()
      window.removeEventListener('pointermove', onMove)
      window.removeEventListener('pointerup', onUp)
    }
    window.addEventListener('pointermove', onMove)
    window.addEventListener('pointerup', onUp)
  }

  /* ── Window resize keeps the stack clamped ── */
  function handleWindowResize() {
    if ($openTools.length > 0) {
      stackWidth = resolveWidth($openTools.length)
    }
  }

  /* ── Init ── */
  onMount(() => {
    try {
      const stored = window.localStorage.getItem(widthPrefKey)
      if (stored) {
        const parsed = parseInt(stored, 10)
        if (!Number.isNaN(parsed)) userOverride = parsed
      }
    } catch {}
    try {
      const storedWidths = window.localStorage.getItem(panelWidthPrefKey)
      if (storedWidths) {
        const parsed = JSON.parse(storedWidths)
        if (parsed && typeof parsed === 'object') panelWidths = parsed
      }
    } catch {}
  })

  /* ── Reactive: sync proportions and width on structural tool changes ── */
  $: syncLayoutForTools($openTools)
  $: if ($openTools.length > 0) {
    stackWidth = resolveWidth($openTools.length)
  }
</script>

<svelte:window on:resize={handleWindowResize} />

{#if $openTools.length > 0}
  <div
    bind:this={panelStack}
    class="panel-stack"
    class:panel-stack-resizing={resizingStack}
    style="width:{stackWidth ?? 0}px"
  >
    <button
      class="panel-resize-handle"
      on:pointerdown={startResize}
      title="Resize tools"
      aria-label="Resize tools"
    ></button>
    {#each $openTools as toolId, index (toolId)}
      <div class="panel-slot" style={getPanelStyle(toolId)}>
        <div class="panel-header">
          <span class="panel-title">{getLabel(toolId)}</span>
          <button class="panel-close" on:click={() => closeTool(toolId)} title="Close {getLabel(toolId)}">
            <Close size={14} />
          </button>
        </div>
        <div class="panel-body">
          <svelte:component this={panelMap[toolId]} />
        </div>
      </div>
      {#if index < $openTools.length - 1}
        <button
          type="button"
          class="panel-divider"
          class:panel-divider-active={activeDividerIndex === index}
          on:pointerdown={(event) => startPanelResize(index, event)}
          title="Resize panels"
          aria-label="Resize panels"
        ></button>
      {/if}
    {/each}
  </div>
{/if}

<style>
  .panel-stack {
    display: flex;
    flex-direction: row;
    align-items: stretch;
    position: relative;
    flex-shrink: 0;
    border-left: 1px solid var(--border);
    background: var(--bg-1);
    overflow: hidden;
    overflow-x: auto;
  }
  .panel-stack-resizing {
    user-select: none;
  }
  .panel-resize-handle {
    position: absolute;
    left: 0;
    top: 0;
    bottom: 0;
    width: 6px;
    padding: 0;
    border: none;
    background: transparent;
    cursor: col-resize;
    z-index: 3;
  }
  .panel-resize-handle::after {
    content: '';
    position: absolute;
    left: 2px;
    top: 0;
    bottom: 0;
    width: 1px;
    background: rgba(255, 255, 255, 0.06);
  }
  .panel-resize-handle:hover::after,
  .panel-stack-resizing .panel-resize-handle::after {
    background: var(--accent);
  }
  .panel-slot {
    min-width: 220px;
    display: flex;
    flex-direction: column;
    min-height: 0;
    overflow: hidden;
  }
  .panel-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 6px 10px;
    background: var(--panel);
    border-bottom: 1px solid var(--border);
    flex-shrink: 0;
  }
  .panel-title {
    font-size: 0.8rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--muted);
  }
  .panel-close {
    background: none;
    border: none;
    color: var(--muted);
    cursor: pointer;
    padding: 2px;
    display: inline-flex;
    align-items: center;
    border-radius: 3px;
  }
  .panel-close:hover {
    color: var(--text);
    background: rgba(255, 255, 255, 0.06);
  }
  .panel-body {
    flex: 1;
    min-height: 0;
    overflow: hidden;
  }
  .panel-divider {
    position: relative;
    flex: 0 0 6px;
    padding: 0;
    border: none;
    background: transparent;
    cursor: col-resize;
    z-index: 2;
  }
  .panel-divider::after {
    content: '';
    position: absolute;
    left: 2px;
    top: 0;
    bottom: 0;
    width: 1px;
    background: rgba(255, 255, 255, 0.08);
  }
  .panel-divider:hover::after,
  .panel-divider-active::after {
    background: var(--accent);
  }
</style>
