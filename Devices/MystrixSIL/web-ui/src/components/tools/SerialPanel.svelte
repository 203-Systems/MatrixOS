<script>
  // Placeholder: Serial/console debugging surface will be added in a later round
</script>

<div class="serial-panel">
  <div class="placeholder">
    <span class="placeholder-title">Serial Console</span>
    <span class="placeholder-desc">Runtime serial stream and console debugging will appear here.</span>
  </div>
</div>

<style>
  .serial-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    padding: 14px;
  }
  .placeholder {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 6px;
    flex: 1;
    color: var(--muted);
    text-align: center;
  }
  .placeholder-title {
    font-size: 0.82rem;
    font-weight: 600;
  }
  .placeholder-desc {
    font-size: 0.75rem;
    max-width: 220px;
    line-height: 1.4;
  }
</style>
