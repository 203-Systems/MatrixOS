<script>
  import { IS_NODE_BACKED } from '../stores/rpc.js'
  import { wsBridgeStatus, RPC_WS_URL } from '../stores/wsbridge.js'

  const apiVersion = '0.1'
  const rpcGroups = [
    'session',
    'runtime',
    'input',
    'led',
    'log',
    'midi',
    'hid',
    'serial',
    'storage.nvs',
  ]

  const statusLabel = {
    unavailable: 'Unavailable',
    connecting: 'Connecting…',
    connected: 'Connected',
    disconnected: 'Reconnecting…',
  }

  const statusClass = {
    unavailable: 'status-error',
    connecting: 'status-idle',
    connected: 'status-live',
    disconnected: 'status-idle',
  }
</script>

<div class="tool-surface">
  <section class="tool-hero">
    <div class="tool-hero-title">Connection</div>
    <div class="tool-hero-desc">
      Remote control, transport visibility, and JSON-RPC entry points for the MystrixSIL runtime.
    </div>
    <div class="tool-tag-row">
      <span class="status-pill status-live">Embedded WASM</span>
      {#if IS_NODE_BACKED}
        <span class="status-pill status-live">Remote Control Available</span>
        <span class="status-pill status-live">JSON-RPC v{apiVersion}</span>
      {:else}
        <span class="status-pill status-idle">Remote Control Unavailable</span>
        <span class="status-pill status-idle">Static Build</span>
      {/if}
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Remote Control</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Primary transport</span>
        <span class="tool-card-value">Embedded WASM</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">JSON-RPC API</span>
        <span class="tool-card-value" class:tool-value-live={IS_NODE_BACKED} class:tool-value-idle={!IS_NODE_BACKED}>
          {IS_NODE_BACKED ? 'Available' : 'Unavailable'}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">External transport</span>
        <span class="tool-card-value tool-value-idle">Node-backed local only</span>
      </div>
    </div>
  </section>

  <section class="tool-section">
    <div class="tool-section-title">JSON-RPC</div>
    {#if IS_NODE_BACKED}
      <div class="tool-list">
        <div class="tool-list-item">
          <div class="tool-list-main">
            <span class="tool-list-title">In-page dispatcher</span>
            <span class="tool-list-detail">Active for this local Node-backed session. Use <code>window.matrixosRpc</code> from DevTools.</span>
          </div>
          <span class="status-pill status-live">Available</span>
        </div>
        <div class="tool-list-item">
          <div class="tool-list-main">
            <span class="tool-list-title">WebSocket server</span>
            <span class="tool-list-detail rpc-ws-url">{RPC_WS_URL}</span>
          </div>
          <span class="status-pill {statusClass[$wsBridgeStatus]}">{statusLabel[$wsBridgeStatus]}</span>
        </div>
        <div class="tool-list-item">
          <div class="tool-list-main">
            <span class="tool-list-title">Handle groups</span>
            <span class="tool-list-detail rpc-groups">{rpcGroups.join(' · ')}</span>
          </div>
          <span class="status-pill status-live">22 handles</span>
        </div>
      </div>

      <div class="rpc-example">
        <div class="rpc-example-label">Quick reference</div>
        <code class="rpc-example-code">await matrixosRpc.call('session.status')</code>
        <code class="rpc-example-code">await matrixosRpc.call('led.getFrame')</code>
        <code class="rpc-example-code">await matrixosRpc.call('input.execute', {"{"} events: [{"{"} input: 'grid:3,3', action: 'Press' {"}"}, {"{"} input: 'grid:3,3', action: 'Release', atMs: 100 {"}"}] {"}"})</code>
        <div class="rpc-example-label ws-label">WebSocket</div>
        <code class="rpc-example-code"># node tools/rpc-test.mjs</code>
      </div>
    {:else}
      <div class="tool-list">
        <div class="tool-list-item">
          <div class="tool-list-main">
            <span class="tool-list-title">JSON-RPC API</span>
            <span class="tool-list-detail">Unavailable in static builds. Use the local Node-backed version for remote control.</span>
          </div>
          <span class="status-pill status-error">Unavailable</span>
        </div>
      </div>
    {/if}
  </section>
</div>

<style>
  .rpc-groups {
    font-family: var(--mono);
    font-size: 0.7rem;
    color: var(--accent);
    opacity: 0.82;
  }
  .rpc-ws-url {
    font-family: var(--mono);
    font-size: 0.72rem;
    color: var(--accent);
    opacity: 0.9;
  }
  .rpc-example {
    margin-top: 10px;
    padding: 10px 12px;
    background: rgba(255, 255, 255, 0.02);
    border: 1px solid var(--border);
    border-radius: 6px;
    display: flex;
    flex-direction: column;
    gap: 5px;
  }
  .rpc-example-label {
    font-size: 0.66rem;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: 0.05em;
  }
  .rpc-example-label.ws-label {
    margin-top: 6px;
  }
  .rpc-example-code {
    font-family: var(--mono);
    font-size: 0.69rem;
    color: var(--text);
    opacity: 0.78;
    white-space: pre-wrap;
    word-break: break-all;
  }
</style>
