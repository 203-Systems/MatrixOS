<script>
  // Placeholder: HID activity inspection will be added in a later round
</script>

<div class="hid-panel">
  <div class="placeholder">
    <span class="placeholder-title">HID Inspector</span>
    <span class="placeholder-desc">Keyboard, gamepad, and raw HID activity will appear here.</span>
  </div>
</div>

<style>
  .hid-panel {
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
