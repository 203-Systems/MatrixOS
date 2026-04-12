<script>
  export let showHero = true
  export let onCloseHero = () => {}
</script>

<div class="tool-surface">
  {#if showHero}
  <section class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">API</div>
    <div class="tool-hero-desc">
      This lane is reserved for the future external debug and automation API surface.
    </div>
  </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Planned surfaces</div>
    <div class="tool-list">
      <div class="tool-list-item">
        <div class="tool-list-main">
          <span class="tool-list-title">External control</span>
          <span class="tool-list-detail">Device control, input injection, and runtime queries will surface here later.</span>
        </div>
      </div>
      <div class="tool-list-item">
        <div class="tool-list-main">
          <span class="tool-list-title">Transport inspector</span>
          <span class="tool-list-detail">Connection state and protocol diagnostics should live here instead of the MatrixOS log stream.</span>
        </div>
      </div>
    </div>
  </section>
</div>
