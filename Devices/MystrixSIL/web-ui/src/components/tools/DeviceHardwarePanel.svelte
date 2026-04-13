<script>
  import { Usb } from 'carbon-icons-svelte'
  import { moduleReady } from '../../stores/wasm.js'
  import { getUsbAvailable, setUsbAvailable } from '../../handles/usb.js'

  export let showHero = true
  export let onCloseHero = () => {}

  let usbConnected = false

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
    <div class="tool-hero-title">Hardware</div>
    <div class="tool-hero-desc">
      Control the USB link state exposed to the MystrixSIL runtime.
    </div>
  </section>
  {/if}

  <div class="tool-section-title">USB Link</div>
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
</style>
