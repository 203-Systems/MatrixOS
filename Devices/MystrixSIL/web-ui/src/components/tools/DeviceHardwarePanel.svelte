<script>
  import { writable } from 'svelte/store'
  import { Usb, BatteryFull, BatteryCharging, Compass } from 'carbon-icons-svelte'
  import { setUsbAvailable } from '../../handles/usb.js'
  export let showHero = true
  export let onCloseHero = () => {}

  const usbConnected = writable(false)
  let batteryPct = 80
  let charging = false

  function toggleUsb() {
    usbConnected.update(v => {
      const next = !v
      setUsbAvailable(next)
      return next
    })
  }
</script>

<div class="device-hw-panel">

  {#if showHero}
  <section class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">Device Hardware</div>
    <div class="tool-hero-desc">
      Simulate hardware peripheral state — USB connection, battery level, and gyroscope. Changes are reflected in the WASM runtime immediately.
    </div>
  </section>
  {/if}

  <!-- USB -->
  <div class="tool-section-title">USB</div>
  <button
    class="hw-strip"
    class:hw-on={$usbConnected}
    on:click={toggleUsb}
    aria-pressed={$usbConnected}
  >
    <div class="hw-strip-left">
      <span class="hw-icon" class:hw-icon-on={$usbConnected}><Usb size={16} /></span>
      <div class="hw-label-stack">
        <span class="hw-title">USB</span>
        <span class="hw-state">{$usbConnected ? 'Connected' : 'Disconnected'}</span>
      </div>
    </div>
    <div class="hw-strip-action">
      {$usbConnected ? 'Disconnect' : 'Connect'}
    </div>
  </button>

  <!-- Battery -->
  <div class="tool-section-title">Battery</div>
  <button
    class="hw-strip hw-strip-battery"
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
      {charging ? 'Disable' : 'Enable'}
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

  <!-- Gyro -->
  <div class="tool-section-title">Gyro</div>
  <div class="hw-wip-row">
    <span class="hw-icon hw-icon-wip"><Compass size={15} /></span>
    <span class="hw-wip-label">Gyroscope simulation — work in progress</span>
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

  /* USB / Charging strip — full-width button */
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
  .hw-strip:hover .hw-strip-action {
    color: var(--accent);
  }

  /* Icons */
  .hw-icon {
    display: flex;
    align-items: center;
    flex-shrink: 0;
    color: var(--muted);
    transition: color 0.15s;
  }
  .hw-icon-on { color: #6bdd8b; }
  .hw-icon-wip { color: var(--muted); opacity: 0.5; }

  /* Label stack */
  .hw-label-stack { display: flex; flex-direction: column; gap: 1px; }
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
  .hw-strip.hw-on .hw-state { color: #6bdd8b; }

  /* Battery level widget */
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
    padding: 0 12px 10px;
    border-top: 1px solid var(--border);
    padding-top: 8px;
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
  .hw-slider:hover::-webkit-slider-thumb { background: var(--accent); }
  .hw-slider::-moz-range-thumb {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--muted);
    border: none;
    cursor: pointer;
  }
  .hw-slider:hover::-moz-range-thumb { background: var(--accent); }

  /* Gyro WIP */
  .hw-wip-row {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 12px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: rgba(255,255,255,0.01);
    opacity: 0.5;
  }
  .hw-wip-label {
    font-size: 0.72rem;
    color: var(--muted);
    font-style: italic;
  }
</style>
