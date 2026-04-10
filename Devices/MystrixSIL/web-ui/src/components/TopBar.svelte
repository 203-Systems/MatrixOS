<script>
  import { Restart } from 'carbon-icons-svelte'
  import { moduleReady, wasmMissing, runtimeStatus, buildIdentity, doReboot } from '../stores/wasm.js'
  import { errorCount, warnCount } from '../stores/logs.js'
</script>

<header class="top-bar">
  <div class="top-bar-left">
    <picture class="top-bar-logo">
      <source srcset="/203dark.svg" media="(prefers-color-scheme: dark)" />
      <img src="/203.svg" alt="203 Systems" height="24" />
    </picture>
    <span class="top-bar-title">Matrix OS Developer Toolkit</span>
    <span class="top-bar-divider">|</span>
    <span class="top-bar-status">
      <span class="status-dot"
        class:dot-live={$moduleReady && !$wasmMissing}
        class:dot-error={$wasmMissing}
        class:dot-loading={!$moduleReady && !$wasmMissing}
      ></span>
      <span class="status-label">{$runtimeStatus}</span>
    </span>
    {#if $errorCount > 0 || $warnCount > 0}
      <span class="top-bar-divider">|</span>
      <span class="top-bar-alerts">
        {#if $errorCount > 0}
          <span class="alert-badge alert-error">{$errorCount} err</span>
        {/if}
        {#if $warnCount > 0}
          <span class="alert-badge alert-warn">{$warnCount} warn</span>
        {/if}
      </span>
    {/if}
  </div>
  <div class="top-bar-right">
    <span class="top-bar-build-id">{$buildIdentity}</span>
    <button class="top-bar-action" on:click={doReboot} title="Reset emulator">
      <Restart size={16} />
      <span>Reset</span>
    </button>
  </div>
</header>

<style>
  .top-bar {
    display: flex;
    align-items: center;
    justify-content: space-between;
    height: 50px;
    padding: 0 16px;
    background: var(--panel);
    border-bottom: 1px solid var(--border);
    font-size: 0.88rem;
    gap: 14px;
    flex-shrink: 0;
    z-index: 10;
  }
  .top-bar-left {
    display: flex;
    align-items: center;
    gap: 12px;
    min-width: 0;
  }
  .top-bar-logo img {
    height: 24px;
    display: block;
  }
  .top-bar-title {
    font-weight: 600;
    font-size: 0.96rem;
    letter-spacing: 0.03em;
    white-space: nowrap;
  }
  .top-bar-divider {
    color: var(--muted);
    font-size: 0.82rem;
    opacity: 0.4;
  }
  .top-bar-status {
    display: flex;
    align-items: center;
    gap: 7px;
  }
  .status-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    flex-shrink: 0;
  }
  .dot-live {
    background: #3dd68c;
    box-shadow: 0 0 6px rgba(61, 214, 140, 0.5);
  }
  .dot-error {
    background: #ff6b6b;
    box-shadow: 0 0 6px rgba(255, 107, 107, 0.5);
  }
  .dot-loading {
    background: #f7c266;
    box-shadow: 0 0 6px rgba(247, 194, 102, 0.4);
    animation: pulse 1.5s ease-in-out infinite;
  }
  @keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.4; }
  }
  .status-label {
    color: var(--muted);
    font-size: 0.84rem;
    white-space: nowrap;
  }
  .top-bar-alerts {
    display: flex;
    gap: 6px;
  }
  .alert-badge {
    font-size: 0.76rem;
    padding: 2px 8px;
    border-radius: 4px;
    font-weight: 500;
  }
  .alert-error {
    background: rgba(255, 107, 107, 0.15);
    color: #ff8b8b;
    border: 1px solid rgba(255, 107, 107, 0.25);
  }
  .alert-warn {
    background: rgba(247, 194, 102, 0.12);
    color: #f7c266;
    border: 1px solid rgba(247, 194, 102, 0.2);
  }
  .top-bar-right {
    display: flex;
    align-items: center;
    gap: 10px;
  }
  .top-bar-build-id {
    color: var(--muted);
    font-size: 0.78rem;
    font-family: var(--mono);
    white-space: nowrap;
    letter-spacing: 0.02em;
  }
  .top-bar-action {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    padding: 4px 12px;
    font-size: 0.82rem;
    font-family: inherit;
    color: var(--text);
    background: var(--bg-2);
    border: 1px solid var(--border);
    border-radius: 5px;
    cursor: pointer;
    transition: border-color 0.15s;
  }
  .top-bar-action:hover {
    border-color: var(--accent);
  }
</style>
