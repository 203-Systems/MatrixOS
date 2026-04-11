<script>
  import { onMount } from 'svelte'
  import { formatBytes, usageHooks, usageSnapshot } from '../../stores/tooling.js'
  import { moduleReady, wasmMissing, runtimeStatus, getUptimeMs } from '../../stores/wasm.js'
  import { errorCount, warnCount, logMessages } from '../../stores/logs.js'

  const apiVersion = '0.1'

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

  onMount(() => {
    uptime = formatUptime(getUptimeMs())
    uptimeTimer = setInterval(() => { uptime = formatUptime(getUptimeMs()) }, 1000)
    return () => clearInterval(uptimeTimer)
  })
</script>

<div class="tool-surface">
  <section class="tool-hero">
    <div class="tool-hero-title">System</div>
    <div class="tool-hero-desc">
      Live runtime status, health counters, and planned telemetry hooks for the MystrixSIL WASM environment.
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Runtime</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Status</span>
        <span class="tool-card-value" class:tool-value-live={$moduleReady} class:tool-value-warn={!$moduleReady}>
          {$runtimeStatus}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">WASM Binary</span>
        <span class="tool-card-value" class:tool-value-live={!$wasmMissing} class:tool-value-error={$wasmMissing}>
          {$wasmMissing ? 'Missing' : 'Present'}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Uptime</span>
        <span class="tool-card-value">{uptime}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Transport</span>
        <span class="tool-card-value sys-muted">WASM (local)</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Health</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">WASM heap</span>
        <span class="tool-card-value">{formatBytes($usageSnapshot.heapBytes)}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Errors</span>
        <span class="tool-card-value" class:tool-value-error={$errorCount > 0}>{$errorCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Warnings</span>
        <span class="tool-card-value" class:tool-value-warn={$warnCount > 0}>{$warnCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Log entries</span>
        <span class="tool-card-value">{$logMessages.length}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Input events</span>
        <span class="tool-card-value">{$usageSnapshot.inputEventCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Open panels</span>
        <span class="tool-card-value">{$usageSnapshot.openToolCount}</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">JSON-RPC API <span class="api-badge">v{apiVersion}</span></div>
    <div class="tool-tag-row">
      <span class="status-pill status-live">Active</span>
      <span class="status-pill status-live">window.matrixosRpc</span>
    </div>
    <div class="tool-list">
      <div class="tool-list-item">
        <div class="tool-list-main">
          <span class="tool-list-title">Transport</span>
          <span class="tool-list-detail">In-page JS dispatcher. Call from DevTools or automation scripts.</span>
        </div>
        <span class="status-pill status-live">Ready</span>
      </div>
      <div class="tool-list-item">
        <div class="tool-list-main">
          <span class="tool-list-title">Methods</span>
          <span class="tool-list-detail api-methods">session · runtime · input · led · log · midi · hid · serial · storage.nvs</span>
        </div>
        <span class="status-pill status-live">22 handles</span>
      </div>
    </div>
    <div class="api-example">
      <div class="api-example-label">Quick reference (DevTools console)</div>
      <code class="api-example-code">await matrixosRpc.call('session.status')</code>
      <code class="api-example-code">await matrixosRpc.call('input.execute', {"{"}events: [{"{"}input:'grid:3,3', action:'Press'}, {"{"}input:'grid:3,3', action:'Release', atMs:100}]{"}"})</code>
      <code class="api-example-code">await matrixosRpc.call('storage.nvs.computeHash', {"{"}text:'settings/wifi'{"}"} )</code>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Future telemetry</div>
    <div class="tool-list">
      {#each usageHooks as hook}
        <div class="tool-list-item">
          <div class="tool-list-main">
            <span class="tool-list-title">{hook.title}</span>
            <span class="tool-list-detail">{hook.detail}</span>
          </div>
          <span class="status-pill status-{hook.status}">{hook.label}</span>
        </div>
      {/each}
    </div>
  </section>
</div>

<style>
  .sys-muted { color: var(--muted); font-size: 0.78rem; }
  .api-badge {
    font-size: 0.65rem;
    font-weight: 400;
    color: var(--muted);
    margin-left: 6px;
    font-family: var(--mono);
  }
  .api-methods {
    font-family: var(--mono);
    font-size: 0.68rem;
    color: var(--accent);
    opacity: 0.8;
  }
  .api-example {
    margin-top: 8px;
    padding: 8px 10px;
    background: rgba(255,255,255,0.02);
    border: 1px solid var(--border);
    border-radius: 6px;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }
  .api-example-label {
    font-size: 0.65rem;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: 0.05em;
    margin-bottom: 2px;
  }
  .api-example-code {
    font-family: var(--mono);
    font-size: 0.68rem;
    color: var(--text);
    opacity: 0.75;
    white-space: pre-wrap;
    word-break: break-all;
  }
</style>
