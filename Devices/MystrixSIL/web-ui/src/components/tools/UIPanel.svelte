<script>
  import { scenarioHooks, uiInspectorHooks, usageSnapshot } from '../../stores/tooling.js'

  function formatInputEvent(event) {
    if (!event) return 'No input captured yet'
    if (event.type === 'fn') return `Function Key ${event.pressed ? 'Press' : 'Release'}`
    return `Grid (${event.x},${event.y}) ${event.pressed ? 'Press' : 'Release'}`
  }
</script>

<div class="tool-surface">
  <section class="tool-hero">
    <div class="tool-hero-title">UI Inspector</div>
    <div class="tool-hero-desc">
      This lane now exposes the live UI-adjacent surfaces already present in MystrixSIL and makes the missing runtime exports explicit instead of leaving a dead placeholder.
    </div>
    <div class="tool-tag-row">
      <span class="status-pill status-live">Event timeline live</span>
      <span class="status-pill status-planned">UI tree export planned</span>
      <span class="status-pill status-partial">Replay path seeded</span>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Live session</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Runtime</span>
        <span class="tool-card-value" class:tool-value-live={$usageSnapshot.runtimeLive} class:tool-value-warn={!$usageSnapshot.runtimeLive}>
          {$usageSnapshot.runtimeStatus}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Open tools</span>
        <span class="tool-card-value">{$usageSnapshot.openToolCount}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Last input</span>
        <span class="tool-card-value">{formatInputEvent($usageSnapshot.lastInputEvent)}</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Inspection hooks</div>
    <div class="tool-list">
      {#each uiInspectorHooks as hook}
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
    <div class="tool-section-title">Scenario / replay path</div>
    <div class="tool-list">
      {#each scenarioHooks as hook}
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
