<script>
  export let showHero = true
  export let onCloseHero = () => {}
</script>

<div class="tool-surface">
  {#if showHero}
  <section class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">Battery</div>
    <div class="tool-hero-desc">
      Battery state monitoring, charge level, and power management inspection.
    </div>
  </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Power state</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Battery</span>
        <span class="tool-card-value tool-value-idle">Not available in SIL</span>
      </div>
    </div>
  </section>
</div>
