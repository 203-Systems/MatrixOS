<script>
  import { onMount, tick } from 'svelte'
  import { Close, Help } from 'carbon-icons-svelte'
  import { openTools, closeTool, deviceTools } from '../stores/tools.js'
  import InputPanel from './InputPanel.svelte'
  import LogsPanel from './LogsPanel.svelte'
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
  import DeviceHardwarePanel from './tools/DeviceHardwarePanel.svelte'

  const panelMap = {
    system: UsagePanel,
    application: ApplicationPanel,
    input: InputPanel,
    logs: LogsPanel,
    ui: UIPanel,
    midi: MIDIPanel,
    hid: HIDPanel,
    serial: SerialPanel,
    usb: USBPanel,
    gyro: GyroPanel,
    battery: BatteryPanel,
    storage: StoragePanel,
    'device-hw': DeviceHardwarePanel,
  }
  // Panels that have a tool-hero (helper) section
  const panelsWithHero = new Set(['system','application','ui','storage','device-hw','midi','hid','serial','input','logs'])

  const LEFT_NAV_W = 60
  const TRAY_W = 48
  const MIN_CENTER = 420
  const MIN_COL = 150

  const DEFAULT_PCT = { 1: 0.30, 2: 0.50, 3: 0.60 }
  const DEFAULT_PCT_MAX = 0.68
  const widthPrefKey = 'matrixos-tool-stack-width'

  let stackWidth = null
  let userOverride = null
  let resizingStack = false
  let renderKey = 'empty'

  function getLabel(id) {
    const tool = deviceTools.find((t) => t.id === id)
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
    const avail = available()
    const minW = Math.max(MIN_COL * Math.max(toolCount, 1), MIN_COL)
    const maxW = Math.max(avail - MIN_CENTER, minW)
    return Math.max(minW, Math.min(maxW, Math.round(px)))
  }

  function resolveWidth(toolCount) {
    const baseWidth = clampWidth(defaultWidth(toolCount), toolCount)
    if (userOverride == null) return baseWidth
    return Math.max(baseWidth, clampWidth(userOverride, toolCount))
  }

  function persistOverride() {
    try {
      if (userOverride != null) window.localStorage.setItem(widthPrefKey, String(userOverride))
      else window.localStorage.removeItem(widthPrefKey)
    } catch {}
  }

  function resetLayoutState() {
    stackWidth = null
    userOverride = null
    renderKey = 'empty'
    persistOverride()
  }

  async function syncWidth(toolIds) {
    if (toolIds.length === 0) {
      resetLayoutState()
      return
    }
    stackWidth = resolveWidth(toolIds.length)
    await tick()
    renderKey = `${toolIds.join('|')}:${stackWidth}`
  }

  function startResize(event) {
    event.preventDefault()
    resizingStack = true
    const onMove = (e) => {
      const newW = window.innerWidth - TRAY_W - e.clientX
      userOverride = clampWidth(newW, $openTools.length)
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

  function handleWindowResize() {
    if ($openTools.length > 0) {
      stackWidth = resolveWidth($openTools.length)
    }
  }

  onMount(() => {
    try {
      const stored = window.localStorage.getItem(widthPrefKey)
      if (stored) {
        const parsed = parseInt(stored, 10)
        if (!Number.isNaN(parsed)) userOverride = parsed
      }
    } catch {}
  })

  $: syncWidth($openTools)

  // Per-panel hero visibility: true = visible
  let helperVisible = {}

  function initHelper(toolId) {
    if (!(toolId in helperVisible)) helperVisible[toolId] = true
    helperVisible = helperVisible
  }

  function closeHelper(toolId) {
    helperVisible[toolId] = false
    helperVisible = helperVisible
  }

  function openHelper(toolId) {
    helperVisible[toolId] = true
    helperVisible = helperVisible
  }

  $: $openTools.forEach(id => initHelper(id))
</script>

<svelte:window on:resize={handleWindowResize} />

{#if $openTools.length > 0}
  {#key renderKey}
    <div
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

      <div
        class="panel-grid"
        style="grid-template-columns:repeat({$openTools.length}, minmax({MIN_COL}px, 1fr))"
      >
        {#each $openTools as toolId (toolId)}
          <section class="panel-slot">
            <div class="panel-header">
              <span class="panel-title">{getLabel(toolId)}</span>
              <div class="panel-header-actions">
                <button
                  class="panel-help-btn"
                  class:panel-help-active={helperVisible[toolId]}
                  on:click={() => helperVisible[toolId] ? closeHelper(toolId) : openHelper(toolId)}
                  title="Help"
                  aria-label="Toggle helper"
                >
                  <Help size={14} />
                </button>
                <button class="panel-close" on:click={() => closeTool(toolId)} title="Close {getLabel(toolId)}">
                  <Close size={14} />
                </button>
              </div>
            </div>
            <div class="panel-body">
              <svelte:component
                this={panelMap[toolId]}
                showHero={helperVisible[toolId]}
                onCloseHero={() => closeHelper(toolId)}
              />
            </div>
          </section>
        {/each}
      </div>
    </div>
  {/key}
{/if}

<style>
  .panel-stack {
    display: flex;
    position: relative;
    flex-shrink: 0;
    border-left: 1px solid var(--border);
    background: var(--bg-1);
    overflow: hidden;
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
  .panel-grid {
    display: grid;
    width: 100%;
    min-width: 0;
  }
  .panel-slot {
    min-width: 0;
    display: flex;
    flex-direction: column;
    min-height: 0;
    overflow: hidden;
    border-right: 1px solid var(--border);
  }
  .panel-slot:last-child {
    border-right: none;
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
  .panel-header-actions {
    display: flex;
    align-items: center;
    gap: 2px;
  }
  .panel-help-btn {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 5px;
    display: inline-flex;
    align-items: center;
  }
  .panel-help-btn:hover {
    color: var(--text);
    border-color: var(--accent);
  }
  .panel-help-active {
    color: var(--accent);
    border-color: rgba(76, 201, 240, 0.4);
  }
  .panel-help-active:hover {
    color: var(--accent);
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
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 5px;
    display: inline-flex;
    align-items: center;
  }
  .panel-close:hover {
    color: var(--text);
    border-color: var(--accent);
  }
  .panel-body {
    flex: 1;
    min-height: 0;
    overflow: hidden;
  }
</style>
