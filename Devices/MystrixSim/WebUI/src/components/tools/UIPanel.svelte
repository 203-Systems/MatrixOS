<script>
  export let showHero = true
  export let onCloseHero = () => {}
</script>

<div class="tool-surface">
  {#if showHero}
  <section class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title-row">
      <div class="tool-hero-title">UI Debug</div>
      <span class="status-pill status-warn">Under Construction</span>
    </div>
    <div class="tool-hero-desc">
      Under construction. This workspace is intended for Matrix OS UI inspection, redraw controls, LED buffer visibility, and scripted UI-debug flows.
    </div>
  </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Display</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">LED Buffer</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Framerate</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Render Calls</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Actions</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Force Redraw</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Screenshot</span>
        <span class="tool-card-value tool-value-idle">Planned</span>
      </div>
    </div>
  </section>
</div>
