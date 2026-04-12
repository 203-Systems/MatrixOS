<script>
  export let showHero = true
  export let onCloseHero = () => {}
</script>

<div class="tool-surface">
  {#if showHero}
  <section class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">UI Debug</div>
    <span class="status-pill status-warn">Under Construction</span>
    <div class="tool-hero-desc">
      Inspect and manipulate the MatrixOS UI layer at runtime. Use this panel to probe display state, trigger UI redraws, inspect the LED buffer, and replay input sequences for UI testing.
    </div>
  </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Display</div>
    <div class="tool-grid">
      <div class="tool-card tool-card-wip">
        <span class="tool-card-label">LED Buffer</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
      <div class="tool-card tool-card-wip">
        <span class="tool-card-label">Framerate</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
      <div class="tool-card tool-card-wip">
        <span class="tool-card-label">Render calls</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Actions</div>
    <div class="tool-grid">
      <div class="tool-card tool-card-wip">
        <span class="tool-card-label">Force redraw</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
      <div class="tool-card tool-card-wip">
        <span class="tool-card-label">Screenshot</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
    </div>
  </section>
</div>

<style>
  .tool-card-wip { opacity: 0.55; }
</style>
