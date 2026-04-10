<script>
  import { moduleReady, wasmMissing, runtimeStatus, getRotation, getUptimeMs, versionLabel } from '../../stores/wasm.js'
  import { onMount } from 'svelte'

  let rotation = 0
  let uptime = '—'
  let timer

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
    timer = setInterval(poll, 1000)
    return () => clearInterval(timer)
  })
</script>

<div class="tool-surface">
  <section class="tool-hero">
    <div class="tool-hero-title">Application</div>
    <div class="tool-hero-desc">
      Application lifecycle inspection: current app, launch history, and runtime state.
    </div>
    <div class="tool-tag-row">
      <span class="status-pill status-partial">Lifecycle tracking</span>
      <span class="status-pill status-planned">Launch history</span>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Runtime</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Status</span>
        <span class="tool-card-value" class:tool-value-live={$moduleReady && !$wasmMissing} class:tool-value-warn={!$moduleReady || $wasmMissing}>
          {$runtimeStatus}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Version</span>
        <span class="tool-card-value">{$versionLabel}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Rotation</span>
        <span class="tool-card-value">{rotation}°</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Uptime</span>
        <span class="tool-card-value">{uptime}</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Active app</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Current</span>
        <span class="tool-card-value tool-value-idle">Not yet exported</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Launch history</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
    </div>
  </section>
</div>
