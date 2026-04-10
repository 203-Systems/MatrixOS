<script>
  import { formatBytes, usageHooks, usageSnapshot } from '../../stores/tooling.js'
</script>

<div class="tool-surface">
  <section class="tool-hero">
    <div class="tool-hero-title">Usage</div>
    <div class="tool-hero-desc">
      Usage now summarizes the live browser-side runtime footprint we can measure today and keeps a dedicated home ready for future allocator, task, and scenario telemetry.
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Live counters</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">WASM heap</span>
        <span class="tool-card-value">{formatBytes($usageSnapshot.heapBytes)}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Log entries</span>
        <span class="tool-card-value">{$usageSnapshot.logCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Input events</span>
        <span class="tool-card-value">{$usageSnapshot.inputEventCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Injected held</span>
        <span class="tool-card-value">{$usageSnapshot.injectedHeldCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Runtime held</span>
        <span class="tool-card-value">{$usageSnapshot.runtimeHeldCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Open panels</span>
        <span class="tool-card-value">{$usageSnapshot.openToolCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Errors</span>
        <span class="tool-card-value" class:tool-value-error={$usageSnapshot.errorCount > 0}>{$usageSnapshot.errorCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Warnings</span>
        <span class="tool-card-value" class:tool-value-warn={$usageSnapshot.warnCount > 0}>{$usageSnapshot.warnCount}</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Current summary</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Runtime state</span>
        <span class="tool-card-value" class:tool-value-live={$usageSnapshot.runtimeLive} class:tool-value-warn={!$usageSnapshot.runtimeLive}>
          {$usageSnapshot.runtimeStatus}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Last log</span>
        <span class="tool-card-value">{$usageSnapshot.lastLogText}</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Future telemetry hooks</div>
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
