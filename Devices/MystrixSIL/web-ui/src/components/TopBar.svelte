<script>
  import { moduleReady, wasmMissing, runtimeStatus, buildIdentity } from '../stores/wasm.js'
  import { errorCount, warnCount } from '../stores/logs.js'
  import { IS_NODE_BACKED } from '../stores/rpc.js'
  import { wsBridgeStatus } from '../stores/wsbridge.js'

  $: runtimeLive = $moduleReady && !$wasmMissing
  $: runtimeLabel = runtimeLive && $runtimeStatus === 'Live' ? 'Runtime Live' : $runtimeStatus
  $: remoteConnected = IS_NODE_BACKED && $wsBridgeStatus === 'connected'
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
        class:dot-live={runtimeLive}
        class:dot-error={$wasmMissing}
        class:dot-loading={!$moduleReady && !$wasmMissing}
      ></span>
      <span class="status-label">{runtimeLabel}</span>
    </span>
    {#if IS_NODE_BACKED}
      <span class="top-bar-divider">|</span>
      <span class="top-bar-remote" class:remote-active={remoteConnected}>
        <span class="remote-dot" class:dot-remote-live={remoteConnected}></span>
        <span class="remote-label">{remoteConnected ? 'Remote Control Connected' : 'Remote Control Ready'}</span>
      </span>
    {/if}
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
  .top-bar-remote {
    display: flex;
    align-items: center;
    gap: 7px;
    min-width: 0;
  }
  .remote-dot {
    width: 8px;
    height: 8px;
    border-radius: 999px;
    background: var(--muted);
    opacity: 0.65;
    flex-shrink: 0;
    transition: background 0.2s, opacity 0.2s, box-shadow 0.2s;
  }
  .dot-remote-live {
    background: var(--accent);
    opacity: 1;
    box-shadow: 0 0 6px rgba(76, 201, 240, 0.5);
  }
  .remote-label {
    color: var(--muted);
    font-size: 0.84rem;
    white-space: nowrap;
    transition: color 0.2s;
  }
  .remote-active .remote-label {
    color: var(--muted);
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
  .top-bar-right { display: flex; align-items: center; }
  .top-bar-build-id {
    color: var(--muted);
    font-size: 0.78rem;
    font-family: var(--mono);
    white-space: nowrap;
    letter-spacing: 0.02em;
  }
</style>
