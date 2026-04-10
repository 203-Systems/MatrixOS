<script>
  import { logMessages } from '../../stores/logs.js'
  import { browserCapabilities, debugHooks, transportHooks, usageSnapshot } from '../../stores/tooling.js'

  $: preview = $logMessages.slice(-12)
</script>

<div class="tool-surface">
  <section class="tool-hero">
    <div class="tool-hero-title">Serial Console</div>
    <div class="tool-hero-desc">
      Serial is now an honest debug-transport surface: it previews the live local console and spells out the separate hooks for remote SDK and editor/debugger integration.
    </div>
    <div class="tool-tag-row">
      <span class="status-pill status-live">Local console live</span>
      <span class="status-pill status-planned">Remote socket planned</span>
      <span class="status-pill status-planned">Debugger lane planned</span>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Transport posture</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Runtime</span>
        <span class="tool-card-value" class:tool-value-live={$usageSnapshot.runtimeLive} class:tool-value-warn={!$usageSnapshot.runtimeLive}>
          {$usageSnapshot.runtimeStatus}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Local console</span>
        <span class="tool-card-value" class:tool-value-live={$usageSnapshot.logCount > 0} class:tool-value-warn={$usageSnapshot.logCount === 0}>
          {$usageSnapshot.logCount > 0 ? 'Streaming via Logs store' : 'Waiting for logs'}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">WebSocket</span>
        <span class="tool-card-value" class:tool-value-live={$browserCapabilities.webSocket} class:tool-value-warn={!$browserCapabilities.webSocket}>
          {$browserCapabilities.detected ? ($browserCapabilities.webSocket ? 'Browser-ready' : 'Unavailable') : 'Detecting'}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">SDK bridge</span>
        <span class="tool-card-value tool-value-warn">Not connected</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Console preview</div>
    <div class="tool-console">
      {#if preview.length === 0}
        <div class="tool-empty">No runtime logs have been captured yet.</div>
      {:else}
        {#each preview as entry (entry.id)}
          <div class="tool-console-row">
            <span class="tool-console-time">{entry.timestamp}</span>
            <span class="tool-console-level">{entry.level}</span>
            <span class="tool-console-text" style={entry.color ? `color:${entry.color}` : ''}>{entry.text}</span>
          </div>
        {/each}
      {/if}
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Transport lanes</div>
    <div class="tool-list">
      {#each transportHooks as hook}
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

  <section class="tool-section">
    <div class="tool-section-title">Debugger adapters</div>
    <div class="tool-list">
      {#each debugHooks as hook}
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
