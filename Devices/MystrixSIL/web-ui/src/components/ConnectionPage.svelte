<div class="tool-surface">
  <section class="tool-hero">
    <div class="tool-hero-title">Connection</div>
    <div class="tool-hero-desc">
      This workspace is reserved for host connection state, remote debug transport, and future SDK attach flows.
    </div>
    <div class="tool-tag-row">
      <span class="status-pill status-live">Local WASM loopback</span>
      <span class="status-pill status-planned">Remote debug bridge</span>
      <span class="status-pill status-planned">SDK transport</span>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Current connection model</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Primary transport</span>
        <span class="tool-card-value">Embedded WASM</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Remote control</span>
        <span class="tool-card-value tool-value-idle">Not attached</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">SDK bridge</span>
        <span class="tool-card-value tool-value-warn">Planned</span>
      </div>
    </div>
  </section>
</div>
