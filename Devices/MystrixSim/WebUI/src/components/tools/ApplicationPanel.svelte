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
      Under construction. This panel is reserved for active app inspection, launch history, and runtime-facing application metadata.
    </div>
  </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Active App</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Current</span>
        <span class="tool-card-value tool-value-idle">Not yet exported</span>
      </div>
    </div>
  </section>
</div>
