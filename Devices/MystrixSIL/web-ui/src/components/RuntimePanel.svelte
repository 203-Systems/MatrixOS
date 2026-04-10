<script>
  import { onMount } from 'svelte'
  import { get } from 'svelte/store'
  import { moduleReady, wasmMissing, runtimeStatus, getRotation, getUptimeMs } from '../stores/wasm.js'
  import { errorCount, warnCount, logMessages } from '../stores/logs.js'

  let rotation = 0
  let uptime = '—'
  let uptimeTimer

  function formatUptime(ms) {
    if (!ms) return '—'
    const s = Math.floor(ms / 1000)
    const m = Math.floor(s / 60)
    const h = Math.floor(m / 60)
    if (h > 0) return `${h}h ${m % 60}m ${s % 60}s`
    if (m > 0) return `${m}m ${s % 60}s`
    return `${s}s`
  }

  function poll() {
    rotation = getRotation()
    uptime = formatUptime(getUptimeMs())
  }

  onMount(() => {
    poll()
    uptimeTimer = setInterval(poll, 1000)
    return () => clearInterval(uptimeTimer)
  })
</script>

<div class="runtime-panel">
  <div class="rt-section">
    <span class="rt-section-title">Status</span>
    <div class="rt-cards">
      <div class="rt-card">
        <span class="rt-card-label">Runtime</span>
        <span class="rt-card-value" class:val-live={$moduleReady} class:val-error={$wasmMissing} class:val-loading={!$moduleReady && !$wasmMissing}>
          {$runtimeStatus}
        </span>
      </div>
      <div class="rt-card">
        <span class="rt-card-label">WASM Binary</span>
        <span class="rt-card-value" class:val-live={!$wasmMissing} class:val-error={$wasmMissing}>
          {$wasmMissing ? 'Missing' : 'Present'}
        </span>
      </div>
      <div class="rt-card">
        <span class="rt-card-label">Rotation</span>
        <span class="rt-card-value">{rotation}°</span>
      </div>
      <div class="rt-card">
        <span class="rt-card-label">Uptime</span>
        <span class="rt-card-value">{uptime}</span>
      </div>
    </div>
  </div>

  <div class="rt-section">
    <span class="rt-section-title">Health</span>
    <div class="rt-cards">
      <div class="rt-card">
        <span class="rt-card-label">Errors</span>
        <span class="rt-card-value" class:val-error={$errorCount > 0}>{$errorCount}</span>
      </div>
      <div class="rt-card">
        <span class="rt-card-label">Warnings</span>
        <span class="rt-card-value" class:val-warn={$warnCount > 0}>{$warnCount}</span>
      </div>
      <div class="rt-card">
        <span class="rt-card-label">Log entries</span>
        <span class="rt-card-value">{$logMessages.length}</span>
      </div>
    </div>
  </div>

  <div class="rt-section">
    <span class="rt-section-title">Connection</span>
    <div class="rt-cards">
      <div class="rt-card rt-card-wide">
        <span class="rt-card-label">Transport</span>
        <span class="rt-card-value rt-muted">WASM (local)</span>
      </div>
      <div class="rt-card rt-card-wide">
        <span class="rt-card-label">Debug API</span>
        <span class="rt-card-value rt-muted">Not connected</span>
      </div>
    </div>
  </div>
</div>

<style>
  .runtime-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    gap: 20px;
    padding: 16px;
    overflow-y: auto;
    scrollbar-width: thin;
    scrollbar-color: rgba(255,255,255,0.12) transparent;
  }
  .rt-section {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }
  .rt-section-title {
    font-size: 0.72rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.08em;
    color: var(--muted);
  }
  .rt-cards {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
    gap: 6px;
  }
  .rt-card {
    display: flex;
    flex-direction: column;
    gap: 2px;
    padding: 10px 12px;
    border-radius: 6px;
    background: var(--bg-2);
    border: 1px solid var(--border);
  }
  .rt-card-wide {
    grid-column: span 2;
  }
  .rt-card-label {
    font-size: 0.7rem;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: 0.04em;
  }
  .rt-card-value {
    font-size: 0.88rem;
    font-family: var(--mono);
    color: var(--text);
  }
  .val-live { color: #3dd68c; }
  .val-error { color: #ff8b8b; }
  .val-warn { color: #f7c266; }
  .val-loading { color: #f7c266; }
  .rt-muted { color: var(--muted); font-size: 0.78rem; }
</style>
