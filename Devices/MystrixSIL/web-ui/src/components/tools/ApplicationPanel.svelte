<script>
  export let showHero = true
  export let onCloseHero = () => {}
</script>

<div class="tool-surface">
  {#if showHero}
  <section class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title-row">
      <div class="tool-hero-title">Application</div>
      <span class="status-pill status-warn">Under Construction</span>
    </div>
    <div class="tool-hero-desc">
      Application lifecycle inspection: current app, launch history, and runtime state.
    </div>
  </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Active app</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Current</span>
        <span class="tool-card-value tool-value-idle">Not yet exported</span>
      </div>
    </div>
  </section>
</div>
