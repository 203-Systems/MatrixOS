<script>
  export let showHero = true
  export let onCloseHero = () => {}
</script>

<div class="tool-surface">
  {#if showHero}
  <section class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">Gyro</div>
    <div class="tool-hero-desc">
      Gyroscope and accelerometer data inspection for motion-aware applications.
    </div>
  </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Sensor state</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">IMU</span>
        <span class="tool-card-value tool-value-idle">Not available in SIL</span>
      </div>
    </div>
  </section>
</div>
