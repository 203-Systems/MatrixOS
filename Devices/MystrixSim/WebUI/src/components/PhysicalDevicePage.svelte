<script>
  import { onMount } from 'svelte'
  import { Power, Usb } from 'carbon-icons-svelte'
  import {
    connectPhysicalDevice,
    disconnectPhysicalDevice,
    initializePhysicalBridge,
    physicalDeviceState,
  } from '../stores/physicalDevice.js'
  import { inputEvents } from '../stores/input.js'

  onMount(() => {
    void initializePhysicalBridge()
  })

  $: isReady = $physicalDeviceState.connected && $physicalDeviceState.developerReady
  $: statusText = (() => {
    if (!$physicalDeviceState.supported) return 'WebHID Unavailable'
    if ($physicalDeviceState.error) return 'Error'
    if (isReady) return 'Connected'
    if ($physicalDeviceState.connecting) return 'Connecting'
    return $physicalDeviceState.connected ? 'Idle' : 'Disconnected'
  })()
  $: deviceText = $physicalDeviceState.deviceName || 'None'
  $: recentInputEvents = $inputEvents.slice(-16).reverse()

  function inputLabel(event) {
    if (event.type === 'fn') return 'Function'
    if (event.type === 'grid') return `Grid ${event.x},${event.y}`
    if (event.type === 'touchbar') return `Touch ${event.x}:${event.y}`
    return event.type || 'Input'
  }

  function inputStateLabel(event) {
    return event.state || (event.pressed ? 'Pressed' : 'Released')
  }

  function inputDetail(event) {
    const parts = []
    if (event.velocity != null) parts.push(`vel ${event.velocity}`)
    if (event.pressure != null) parts.push(`press ${event.pressure}`)
    return parts.length > 0 ? parts.join(' / ') : '-'
  }
</script>

<div class="tool-surface physical-page">
  <section class="tool-hero">
    <div class="tool-hero-title">Hardware</div>
    <div class="tool-hero-desc">Connect a physical Mystrix through WebHID for display output and key input.</div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Active Hardware</div>
    <div class="physical-current-layout">
      <div class="tool-grid physical-current-grid">
        <div class="tool-card physical-device-card">
          <span class="tool-card-label">Device</span>
          <span class="tool-card-value">{deviceText}</span>
        </div>
        <div class="tool-card">
          <span class="tool-card-label">Status</span>
          <span
            class="tool-card-value"
            class:physical-value-live={isReady}
            class:physical-value-error={Boolean($physicalDeviceState.error)}
          >{statusText}</span>
        </div>
        <div class="tool-card">
          <span class="tool-card-label">LED Frames</span>
          <span class="tool-card-value">{$physicalDeviceState.mirroredFrames}</span>
        </div>
        <div class="tool-card">
          <span class="tool-card-label">Input Events</span>
          <span class="tool-card-value">{$physicalDeviceState.inputEvents}</span>
        </div>
      </div>

      <div class="physical-action-panel">
        {#if $physicalDeviceState.connected}
          <button class="physical-action physical-action-muted" on:click={disconnectPhysicalDevice}>
            <Power size={18} />
            <span>Disconnect</span>
          </button>
        {:else}
          <button
            class="physical-action"
            disabled={!$physicalDeviceState.supported || $physicalDeviceState.connecting}
            on:click={connectPhysicalDevice}
          >
            <Usb size={18} />
            <span>{$physicalDeviceState.connecting ? 'Connecting' : 'Connect'}</span>
          </button>
        {/if}
      </div>
    </div>

    {#if $physicalDeviceState.error}
      <div class="physical-error-note">{$physicalDeviceState.error}</div>
    {/if}
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Input Log</div>
    <div class="physical-input-log">
      {#if recentInputEvents.length === 0}
        <div class="physical-input-empty">No input yet</div>
      {:else}
        {#each recentInputEvents as event (event.id)}
          <div class="physical-input-row">
            <span class="physical-input-time">{event.timestamp}</span>
            <span class="physical-input-name">{inputLabel(event)}</span>
            <span
              class:physical-input-press={event.pressed}
              class:physical-input-aftertouch={inputStateLabel(event) === 'Aftertouch'}
              class="physical-input-state"
            >
              {inputStateLabel(event)}
            </span>
            <span class="physical-input-detail">{inputDetail(event)}</span>
          </div>
        {/each}
      {/if}
    </div>
  </section>
</div>

<style>
  .physical-page {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }
  .physical-current-layout {
    display: flex;
    align-items: stretch;
    gap: 12px;
  }
  .physical-current-grid {
    flex: 1;
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }
  .physical-device-card {
    grid-column: span 3;
  }
  .physical-action-panel {
    width: 260px;
    display: flex;
    align-items: stretch;
  }
  .physical-action {
    width: 100%;
    min-height: 100%;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    border: 1px solid rgba(76, 201, 240, 0.45);
    color: #b9ebff;
    background: rgba(76, 201, 240, 0.1);
    border-radius: 8px;
    font: inherit;
    font-size: 0.92rem;
    font-weight: 800;
    cursor: pointer;
    text-transform: uppercase;
    letter-spacing: 0.03em;
    transition: background 0.12s, border-color 0.12s, opacity 0.12s;
  }
  .physical-action:hover {
    background: rgba(76, 201, 240, 0.16);
    border-color: rgba(76, 201, 240, 0.7);
  }
  .physical-action:disabled {
    cursor: not-allowed;
    opacity: 0.55;
  }
  .physical-action-muted {
    color: var(--muted);
    border-color: rgba(255, 255, 255, 0.16);
    background: rgba(255, 255, 255, 0.04);
  }
  .physical-value-live {
    color: #6bdd8b;
  }
  .physical-value-error {
    color: #ff8f8f;
  }
  .physical-error-note {
    margin-top: 12px;
    padding: 10px 12px;
    border: 1px solid rgba(255, 143, 143, 0.22);
    border-radius: 8px;
    color: #ffb2b2;
    background: rgba(255, 143, 143, 0.08);
    font-size: 0.86rem;
    line-height: 1.4;
    word-break: break-word;
  }
  .physical-input-log {
    display: flex;
    flex-direction: column;
    min-height: 180px;
    max-height: 280px;
    overflow: auto;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.03);
  }
  .physical-input-row,
  .physical-input-empty {
    display: grid;
    grid-template-columns: 96px minmax(0, 1fr) 92px 128px;
    gap: 10px;
    align-items: center;
    padding: 8px 10px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.07);
    font-size: 0.82rem;
  }
  .physical-input-row:last-child {
    border-bottom: 0;
  }
  .physical-input-empty {
    display: block;
    color: var(--muted);
  }
  .physical-input-time {
    color: var(--muted);
    font-variant-numeric: tabular-nums;
  }
  .physical-input-name {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }
  .physical-input-state {
    color: #ffb2b2;
    font-weight: 700;
  }
  .physical-input-press {
    color: #6bdd8b;
  }
  .physical-input-aftertouch {
    color: #f7c266;
  }
  .physical-input-detail {
    justify-self: end;
    color: var(--muted);
    font-variant-numeric: tabular-nums;
    white-space: nowrap;
  }
  @media (max-width: 1100px) {
    .physical-current-layout {
      flex-direction: column;
    }
    .physical-current-grid {
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }
    .physical-device-card {
      grid-column: span 2;
    }
    .physical-action-panel {
      width: 100%;
      min-height: 58px;
    }
  }
  @media (max-width: 640px) {
    .physical-current-grid {
      grid-template-columns: 1fr;
    }
    .physical-device-card {
      grid-column: span 1;
    }
    .physical-action {
      min-height: 52px;
    }
    .physical-input-row,
    .physical-input-empty {
      grid-template-columns: 82px minmax(0, 1fr) 84px;
      gap: 8px;
      font-size: 0.78rem;
    }
    .physical-input-detail {
      grid-column: 2 / -1;
      justify-self: start;
    }
  }
</style>
