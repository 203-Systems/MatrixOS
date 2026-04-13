<script>
  import { Usb, BatteryCharging, BatteryFull, Compass } from 'carbon-icons-svelte'
  import { moduleReady } from '../../stores/wasm.js'
  import { getUsbAvailable, setUsbAvailable } from '../../handles/usb.js'

  export let showHero = true
  export let onCloseHero = () => {}

  let usbConnected = false
  let batteryPct = 80
  let charging = false

  $: if ($moduleReady) {
    const current = getUsbAvailable()
    if (current != null) {
      usbConnected = current
    }
  }

  function toggleUsb() {
    const next = !usbConnected
    if (setUsbAvailable(next)) {
      usbConnected = next
    }
  }
</script>

<div class="device-hw-panel">
  {#if showHero}
  <section class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title-row">
      <div class="tool-hero-title">Hardware</div>
      <span class="status-pill status-warn">Under Construction</span>
    </div>
    <div class="tool-hero-desc">
      Control the USB link state exposed to the MystrixSIL runtime and stage battery / gyro simulation work in progress.
    </div>
  </section>
  {/if}

  <div class="tool-section-title">USB</div>
  <button
    class="hw-strip"
    class:hw-on={usbConnected}
    on:click={toggleUsb}
    aria-pressed={usbConnected}
    disabled={!$moduleReady}
  >
    <div class="hw-strip-left">
      <span class="hw-icon" class:hw-icon-on={usbConnected}><Usb size={16} /></span>
      <div class="hw-label-stack">
        <span class="hw-title">USB</span>
        <span class="hw-state">{usbConnected ? 'Connected' : 'Disconnected'}</span>
      </div>
    </div>
    <div class="hw-strip-action">
      {usbConnected ? 'Disconnect' : 'Connect'}
    </div>
  </button>

  <div class="tool-section-title-row hw-section-title-row">
    <div class="tool-section-title hw-section-title">Battery</div>
    <span class="status-pill status-warn">Under Construction</span>
  </div>
  <button
    class="hw-strip"
    class:hw-on={charging}
    on:click={() => charging = !charging}
    aria-pressed={charging}
  >
    <div class="hw-strip-left">
      <span class="hw-icon" class:hw-icon-on={charging}><BatteryCharging size={16} /></span>
      <div class="hw-label-stack">
        <span class="hw-title">Charging</span>
        <span class="hw-state">{charging ? 'On' : 'Off'}</span>
      </div>
    </div>
    <div class="hw-strip-action">
      {charging ? 'Disconnect' : 'Connect'}
    </div>
  </button>

  <div class="hw-level-strip">
    <div class="hw-level-header">
      <div class="hw-strip-left">
        <span class="hw-icon"><BatteryFull size={16} /></span>
        <div class="hw-label-stack">
          <span class="hw-title">Battery Level</span>
        </div>
      </div>
      <span class="hw-level-pct">{batteryPct}%</span>
    </div>
    <div class="hw-level-slider-row">
      <input class="hw-slider" type="range" min="0" max="100" bind:value={batteryPct} />
    </div>
  </div>

  <div class="tool-section-title-row hw-section-title-row">
    <div class="tool-section-title hw-section-title">Gyro</div>
    <span class="status-pill status-warn">Under Construction</span>
  </div>
  <div class="hw-wip-row">
    <span class="hw-icon hw-icon-wip"><Compass size={15} /></span>
    <span class="hw-wip-label">Gyroscope simulation is under construction.</span>
  </div>
</div>

<style>
  .device-hw-panel {
    display: flex;
    flex-direction: column;
    padding: 10px;
    gap: 4px;
  }

  .tool-section-title {
    font-size: 0.68rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--muted);
    padding: 8px 2px 4px;
  }

  .hw-section-title-row {
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 10px 2px 6px;
  }

  .hw-section-title {
    padding: 0;
  }

  .hw-section-title-row :global(.status-pill) {
    align-self: center;
  }

  .hw-strip {
    display: grid;
    grid-template-columns: 1fr auto;
    align-items: center;
    border-radius: 6px;
    border: 1px solid var(--border);
    background: rgba(255, 255, 255, 0.02);
    overflow: hidden;
    transition: background 0.2s, border-color 0.2s;
    width: 100%;
    text-align: left;
    font-family: inherit;
    cursor: pointer;
    padding: 0;
  }

  .hw-strip:disabled {
    cursor: default;
    opacity: 0.55;
  }

  .hw-strip.hw-on {
    border-color: rgba(107, 221, 139, 0.28);
    background: rgba(107, 221, 139, 0.07);
  }

  .hw-strip-left {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 9px 12px;
  }

  .hw-strip-action {
    align-self: stretch;
    min-width: 90px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: var(--muted);
    font-size: 0.7rem;
    font-weight: 600;
    white-space: nowrap;
    padding: 0 14px;
    transition: color 0.15s;
  }

  .hw-strip:not(:disabled):hover .hw-strip-action {
    color: var(--accent);
  }

  .hw-icon {
    display: flex;
    align-items: center;
    flex-shrink: 0;
    color: var(--muted);
    transition: color 0.15s;
  }

  .hw-icon-on {
    color: #6bdd8b;
  }

  .hw-icon-wip {
    opacity: 0.5;
  }

  .hw-label-stack {
    display: flex;
    flex-direction: column;
    gap: 1px;
  }

  .hw-title {
    font-size: 0.78rem;
    font-weight: 600;
    color: var(--text);
    text-transform: uppercase;
    letter-spacing: 0.04em;
  }

  .hw-state {
    font-size: 0.68rem;
    color: var(--muted);
    font-family: var(--mono);
    transition: color 0.15s;
  }

  .hw-strip.hw-on .hw-state {
    color: #6bdd8b;
  }

  .hw-level-strip {
    display: flex;
    flex-direction: column;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: rgba(255, 255, 255, 0.02);
    overflow: hidden;
  }

  .hw-level-header {
    display: grid;
    grid-template-columns: 1fr auto;
    align-items: center;
  }

  .hw-level-pct {
    font-size: 0.76rem;
    font-family: var(--mono);
    color: var(--muted);
    padding: 0 14px;
    white-space: nowrap;
  }

  .hw-level-slider-row {
    padding: 8px 12px 10px;
    border-top: 1px solid var(--border);
  }

  .hw-slider {
    width: 100%;
    cursor: pointer;
    display: block;
    -webkit-appearance: none;
    appearance: none;
    height: 2px;
    border-radius: 1px;
    background: var(--border);
    outline: none;
  }

  .hw-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--muted);
    cursor: pointer;
    transition: background 0.15s;
  }

  .hw-slider:hover::-webkit-slider-thumb {
    background: var(--accent);
  }

  .hw-slider::-moz-range-thumb {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--muted);
    border: none;
    cursor: pointer;
  }

  .hw-slider:hover::-moz-range-thumb {
    background: var(--accent);
  }

  .hw-wip-row {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 12px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: rgba(255,255,255,0.01);
    opacity: 0.6;
  }

  .hw-wip-label {
    font-size: 0.72rem;
    color: var(--muted);
    font-style: italic;
  }
</style>
