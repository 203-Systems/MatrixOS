<script>
  import { writable } from 'svelte/store'
  import { Usb } from 'carbon-icons-svelte'

  const usbConnected = writable(false)

  function toggleUsb() {
    usbConnected.update(v => {
      const next = !v
      const mod = window.Module
      if (mod?._MatrixOS_Wasm_SetUsbAvailable) {
        mod._MatrixOS_Wasm_SetUsbAvailable(next ? 1 : 0)
      }
      return next
    })
  }
</script>

<div class="usb-panel">
  <div class="usb-status-card" class:usb-on={$usbConnected}>
    <div class="usb-left">
      <div class="usb-icon" class:icon-on={$usbConnected}>
        <Usb size={18} />
      </div>
      <div class="usb-label-stack">
        <span class="usb-title">USB</span>
        <span class="usb-state">{$usbConnected ? 'Connected' : 'Disconnected'}</span>
      </div>
    </div>
    <button
      class="usb-toggle-btn"
      class:usb-toggle-on={$usbConnected}
      on:click={toggleUsb}
      aria-pressed={$usbConnected}
    >
      {$usbConnected ? 'Disconnect' : 'Connect'}
    </button>
  </div>
</div>

<style>
  .usb-panel {
    display: flex;
    flex-direction: column;
    padding: 14px;
    height: 100%;
    gap: 10px;
  }

  .usb-status-card {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    padding: 12px 14px;
    border-radius: 8px;
    border: 1px solid var(--border);
    background: rgba(255, 255, 255, 0.02);
    transition: border-color 0.2s, background 0.2s;
  }

  .usb-on {
    border-color: rgba(107, 221, 139, 0.3);
    background: rgba(107, 221, 139, 0.04);
  }

  .usb-left {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .usb-icon {
    color: var(--muted);
    display: flex;
    align-items: center;
    transition: color 0.2s;
  }

  .icon-on {
    color: #6bdd8b;
  }

  .usb-label-stack {
    display: flex;
    flex-direction: column;
    gap: 1px;
  }

  .usb-title {
    font-size: 0.82rem;
    font-weight: 600;
    color: var(--text);
    letter-spacing: 0.04em;
    text-transform: uppercase;
  }

  .usb-state {
    font-size: 0.72rem;
    color: var(--muted);
    font-family: var(--mono);
  }

  .usb-on .usb-state {
    color: #6bdd8b;
  }

  .usb-toggle-btn {
    padding: 6px 14px;
    border-radius: 5px;
    border: 1px solid var(--border);
    background: rgba(255, 255, 255, 0.03);
    color: var(--muted);
    font-family: var(--sans);
    font-size: 0.76rem;
    font-weight: 600;
    cursor: pointer;
    flex-shrink: 0;
    transition: border-color 0.15s, background 0.15s, color 0.15s;
  }

  .usb-toggle-btn:hover {
    border-color: rgba(76, 201, 240, 0.35);
    color: var(--text);
    background: rgba(76, 201, 240, 0.05);
  }

  .usb-toggle-on {
    border-color: rgba(255, 100, 100, 0.35);
    color: #ff8b8b;
    background: rgba(255, 100, 100, 0.04);
  }

  .usb-toggle-on:hover {
    border-color: rgba(255, 100, 100, 0.55);
    background: rgba(255, 100, 100, 0.08);
    color: #ffaaaa;
  }
</style>
