<script>
  import { Wifi, Copy } from 'carbon-icons-svelte'
  import { IS_NODE_BACKED, rpcApi } from '../stores/rpc.js'
  import { wsBridgeStatus, wsBridgeConnectionCount, RPC_WS_URL } from '../stores/wsbridge.js'

  const apiVersion = '0.1'

  const statusLabel = {
    unavailable: 'Unavailable',
    available: 'Available',
    connected: 'Connected',
  }

  const statusClass = {
    unavailable: 'status-error',
    available: 'status-live',
    connected: 'status-live',
  }

  $: wsStatusText = (() => {
    if (!IS_NODE_BACKED) return 'Unavailable'
    if ($wsBridgeStatus === 'connected') {
      return `${statusLabel[$wsBridgeStatus]}: ${$wsBridgeConnectionCount}`
    }
    return statusLabel[$wsBridgeStatus]
  })()

  const namespaces = [
    {
      id: 'bridge',
      summary: 'External WebSocket bridge diagnostics.',
      handles: [
        {
          name: 'bridge.status',
          purpose: 'Return bridge status even when no runtime tab is connected.',
          params: '{}',
          result: '{ runtimeConnected, externalConnectionCount, pendingRequestCount, pendingRequests, subscriptionCount }',
          example: "await matrixosRpc.call('bridge.status')",
        },
      ],
    },
    {
      id: 'session',
      summary: 'Session health and reset control.',
      handles: [
        {
          name: 'session.status',
          purpose: 'Return high-level session status and build metadata.',
          params: '{}',
          result: '{ protocolVersion, sessionId, connected, runtimeReady, build }',
          example: "await matrixosRpc.call('session.status')",
        },
        {
          name: 'session.ping',
          purpose: 'Simple health check.',
          params: '{}',
          result: '{ ok: true }',
          example: "await matrixosRpc.call('session.ping')",
        },
        {
          name: 'session.reset',
          purpose: 'Reset the runtime to a clean startup state.',
          params: '{}',
          result: '{ ok: true }',
          example: "await matrixosRpc.call('session.reset')",
        },
      ],
    },
    {
      id: 'runtime',
      summary: 'Runtime state snapshots.',
      handles: [
        {
          name: 'runtime.getState',
          purpose: 'Return runtime status, uptime, app summary, and warning/error counts.',
          params: '{}',
          result: '{ status, uptimeMs, usbConnected, activeApp, warningCount, errorCount }',
          example: "await matrixosRpc.call('runtime.getState')",
        },
        {
          name: 'runtime.getAppState',
          purpose: 'Return the current active app summary when available.',
          params: '{}',
          result: '{ activeApp }',
          example: "await matrixosRpc.call('runtime.getAppState')",
        },
      ],
    },
    {
      id: 'application',
      summary: 'Application listing and launch control.',
      handles: [
        {
          name: 'application.list',
          purpose: 'Return the active app and launchable app list.',
          params: '{}',
          result: '{ activeApp, activeAppId, applications, available }',
          example: "await matrixosRpc.call('application.list')",
        },
        {
          name: 'application.launch',
          purpose: 'Launch an app by numeric or hex app id.',
          params: "{ appId: '0xBCC0FAEF' }",
          result: '{ ok: true }',
          example: "await matrixosRpc.call('application.launch', { appId: '0xBCC0FAEF' })",
        },
      ],
    },
    {
      id: 'input',
      summary: 'Input listing, injection, and current pressed-state queries.',
      handles: [
        {
          name: 'input.list',
          purpose: 'List canonical input ids such as Function Key and grid keys.',
          params: '{}',
          result: '{ inputs: [...] }',
          example: "await matrixosRpc.call('input.list')",
        },
        {
          name: 'input.execute',
          purpose: 'Inject single actions, combos, or delayed sequences.',
          params: "{ events: [{ input: 'function', action: 'Press' }, { input: 'function', action: 'Release', atMs: 120 }] }",
          result: '{ ok, accepted }',
          example: "await matrixosRpc.call('input.execute', { events: [{ input: 'grid:3,3', action: 'Press' }, { input: 'grid:3,3', action: 'Release', atMs: 100 }] })",
        },
        {
          name: 'input.get',
          purpose: 'Return the currently active inputs.',
          params: '{}',
          result: '{ activeInputs: [...] }',
          example: "await matrixosRpc.call('input.get')",
        },
        {
          name: 'input.releaseAll',
          purpose: 'Release every addressable input before starting another automated interaction.',
          params: '{}',
          result: '{ ok, released }',
          example: "await matrixosRpc.call('input.releaseAll')",
        },
      ],
    },
    {
      id: 'led',
      summary: 'Framebuffer and individual LED inspection.',
      handles: [
        {
          name: 'led.getFrame',
          purpose: 'Return the full grid and underglow LED frame.',
          params: '{}',
          result: '{ timestamp, format, grid, underglow }',
          example: "await matrixosRpc.call('led.getFrame')",
        },
        {
          name: 'led.get',
          purpose: 'Read one LED by id.',
          params: "{ id: 'grid:0,0' }",
          result: '{ id, color }',
          example: "await matrixosRpc.call('led.get', { id: 'grid:0,0' })",
        },
      ],
    },
    {
      id: 'log',
      summary: 'Runtime logs and host-side emulator errors.',
      handles: [
        {
          name: 'log.get',
          purpose: 'Read MatrixOS/runtime log entries with filters.',
          params: "{ last: 20, level: ['warning', 'error'] }",
          result: '{ entries: [...] }',
          example: "await matrixosRpc.call('log.get', { last: 20, level: ['warning', 'error'] })",
        },
        {
          name: 'emulator.getErrors',
          purpose: 'Read host-side JS/emulator errors.',
          params: "{ last: 20 }",
          result: '{ entries: [...] }',
          example: "await matrixosRpc.call('emulator.getErrors', { last: 20 })",
        },
      ],
    },
    {
      id: 'midi',
      summary: 'MIDI ports, sending, and subscriptions.',
      handles: [
        {
          name: 'midi.listPorts',
          purpose: 'List MIDI ports exposed by the runtime.',
          params: '{}',
          result: '{ ports: [...] }',
          example: "await matrixosRpc.call('midi.listPorts')",
        },
        {
          name: 'midi.send',
          purpose: 'Send an external MIDI message into MatrixOS.',
          params: "{ message: { kind: 'noteOn', channel: 1, note: 60, velocity: 100 } }",
          result: '{ ok: true }',
          example: "await matrixosRpc.call('midi.send', { message: { kind: 'noteOn', channel: 1, note: 60, velocity: 100 } })",
        },
        {
          name: 'midi.sendToPort',
          purpose: 'Send a MatrixOS-originated MIDI message to a target port.',
          params: "{ targetPort: '0x0100', message: { kind: 'noteOn', channel: 1, note: 60, velocity: 100 } }",
          result: '{ ok: true }',
          example: "await matrixosRpc.call('midi.sendToPort', { targetPort: '0x0100', message: { kind: 'noteOn', channel: 1, note: 60, velocity: 100 } })",
        },
        {
          name: 'midi.sendSysEx',
          purpose: 'Send a complete external SysEx message into MatrixOS.',
          params: "{ encoding: 'hex', payload: 'F0 00 02 03 4D 58 00 F7' }",
          result: '{ ok: true }',
          example: "await matrixosRpc.call('midi.sendSysEx', { payload: 'F0 00 02 03 4D 58 00 F7' })",
        },
        {
          name: 'midi.sendSysExToPort',
          purpose: 'Send a MatrixOS-originated SysEx message to a target port.',
          params: "{ targetPort: '0x0100', encoding: 'hex', payload: 'F0 00 02 03 4D 58 00 F7' }",
          result: '{ ok: true }',
          example: "await matrixosRpc.call('midi.sendSysExToPort', { targetPort: '0x0100', payload: 'F0 00 02 03 4D 58 00 F7' })",
        },
        {
          name: 'midi.subscribe',
          purpose: 'Subscribe to MIDI traffic notifications.',
          params: '{ } with callback on the caller side',
          result: '{ ok: true }',
          example: "await matrixosRpc.subscribe('midi', callback)",
        },
      ],
    },
    {
      id: 'hid',
      summary: 'Raw HID send path and event subscription.',
      handles: [
        {
          name: 'hid.send',
          purpose: 'Inject raw HID payloads.',
          params: "{ kind: 'rawHid', encoding: 'hex', payload: '01 02 03 04' }",
          result: '{ ok: true }',
          example: "await matrixosRpc.call('hid.send', { kind: 'rawHid', encoding: 'hex', payload: '01 02 03 04' })",
        },
        {
          name: 'hid.subscribe',
          purpose: 'Subscribe to HID monitor events.',
          params: '{ } with callback on the caller side',
          result: '{ ok: true }',
          example: "await matrixosRpc.subscribe('hid', callback)",
        },
      ],
    },
    {
      id: 'serial',
      summary: 'Serial send path and event subscription.',
      handles: [
        {
          name: 'serial.send',
          purpose: 'Send text or hex payloads to the serial path.',
          params: "{ encoding: 'utf8', payload: 'hello from rpc' }",
          result: '{ ok: true }',
          example: "await matrixosRpc.call('serial.send', { encoding: 'utf8', payload: 'hello from rpc' })",
        },
        {
          name: 'serial.subscribe',
          purpose: 'Subscribe to serial monitor events.',
          params: '{ } with callback on the caller side',
          result: '{ ok: true }',
          example: "await matrixosRpc.subscribe('serial', callback)",
        },
      ],
    },
    {
      id: 'python',
      summary: 'Python app launch, input, and output subscription.',
      handles: [
        {
          name: 'python.status',
          purpose: 'Return Python availability, current session mode, and debug summary.',
          params: '{}',
          result: '{ available, active, mode, debug }',
          example: "await matrixosRpc.call('python.status')",
        },
        {
          name: 'python.debug',
          purpose: 'Return Python runtime loop state, staged script metadata, and MicroPython heap/stack summary.',
          params: '{}',
          result: '{ available, active, mode, scriptPath, scripts, runtime }',
          example: "await matrixosRpc.call('python.debug')",
        },
        {
          name: 'python.runText',
          purpose: 'Stage and run a Python script in app mode.',
          params: "{ name: 'pixel_art.py', text: 'print(1)' }",
          result: '{ ok, name, size, mode }',
          example: "await matrixosRpc.call('python.runText', { name: 'pixel_art.py', text })",
        },
        {
          name: 'python.input',
          purpose: 'Send text into the Python REPL or running Python input stream.',
          params: "{ text: 'print(1)\\n' }",
          result: '{ ok: true }',
          example: "await matrixosRpc.call('python.input', { text: 'print(1)\\n' })",
        },
        {
          name: 'python.getOutput',
          purpose: 'Read captured Python output history.',
          params: "{ last: 20, mode: 'app' }",
          result: '{ entries: [...] }',
          example: "await matrixosRpc.call('python.getOutput', { last: 20 })",
        },
        {
          name: 'python.subscribe',
          purpose: 'Subscribe to Python stdout/stderr stream events.',
          params: '{ } with callback on the caller side',
          result: '{ ok: true }',
          example: "await matrixosRpc.subscribe('python', callback)",
        },
      ],
    },
    {
      id: 'storage',
      summary: 'Storage-related APIs for NVS and the browser-backed virtual filesystem.',
      handles: [
        {
          name: 'NVS · storage.nvs.find',
          purpose: 'Find NVS entries by hash filter.',
          params: "{ hash: '0x1234' }",
          result: '{ entries: [...] }',
          example: "await matrixosRpc.call('storage.nvs.find', { hash: '0x1234' })",
        },
        {
          name: 'NVS · storage.nvs.get',
          purpose: 'Read one NVS entry by hash.',
          params: "{ hash: '0x12345678' }",
          result: '{ hash, size, valuePreview, rawBytes }',
          example: "await matrixosRpc.call('storage.nvs.get', { hash: '0x12345678' })",
        },
        {
          name: 'NVS · storage.nvs.set',
          purpose: 'Write one NVS entry by hash.',
          params: "{ hash: '0x12345678', encoding: 'hex', value: '01 02 03 04' }",
          result: '{ ok: true }',
          example: "await matrixosRpc.call('storage.nvs.set', { hash: '0x12345678', encoding: 'hex', value: '01 02 03 04' })",
        },
        {
          name: 'NVS · storage.nvs.computeHash',
          purpose: 'Compute the runtime NVS hash from a key string.',
          params: "{ text: 'settings/wifi' }",
          result: '{ text, hash }',
          example: "await matrixosRpc.call('storage.nvs.computeHash', { text: 'settings/wifi' })",
        },
        {
          name: 'Filesystem · storage.fs.status',
          purpose: 'Return whether the virtual filesystem is mounted and reachable.',
          params: '{}',
          result: '{ mounted }',
          example: "await matrixosRpc.call('storage.fs.status')",
        },
        {
          name: 'Filesystem · storage.fs.list',
          purpose: 'List a directory in the virtual filesystem.',
          params: "{ path: '/' }",
          result: '{ ok, path, entries: [{ name, path, isDir, size }] }',
          example: "await matrixosRpc.call('storage.fs.list', { path: '/' })",
        },
        {
          name: 'Filesystem · storage.fs.read',
          purpose: 'Read a file as utf8, hex, or base64.',
          params: "{ path: '/notes/test.txt', encoding: 'utf8' }",
          result: '{ path, size, encoding, data }',
          example: "await matrixosRpc.call('storage.fs.read', { path: '/notes/test.txt', encoding: 'utf8' })",
        },
        {
          name: 'Filesystem · storage.fs.write',
          purpose: 'Write a file using utf8, hex, base64, or raw byte-array input.',
          params: "{ path: '/notes/test.txt', encoding: 'utf8', data: 'hello from rpc' }",
          result: '{ ok, path, size }',
          example: "await matrixosRpc.call('storage.fs.write', { path: '/notes/test.txt', encoding: 'utf8', data: 'hello from rpc' })",
        },
        {
          name: 'Filesystem · storage.fs.delete',
          purpose: 'Delete a file or an empty directory.',
          params: "{ path: '/notes/test.txt' }",
          result: '{ ok, path }',
          example: "await matrixosRpc.call('storage.fs.delete', { path: '/notes/test.txt' })",
        },
        {
          name: 'Filesystem · storage.fs.mkdir',
          purpose: 'Create a directory, including missing parent folders.',
          params: "{ path: '/notes/archive' }",
          result: '{ ok, path }',
          example: "await matrixosRpc.call('storage.fs.mkdir', { path: '/notes/archive' })",
        },
      ],
    },
  ]

  let rpcMethod = 'session.status'
  let rpcParamsText = '{}'
  let rpcRunning = false
  let rpcResultText = ''
  let rpcErrorText = ''

  async function runInPageRpc() {
    rpcRunning = true
    rpcErrorText = ''
    rpcResultText = ''

    try {
      const params = rpcParamsText.trim() ? JSON.parse(rpcParamsText) : {}
      const response = await rpcApi.call(rpcMethod.trim(), params)
      if (response?.error) {
        rpcErrorText = JSON.stringify(response.error, null, 2)
      } else {
        rpcResultText = JSON.stringify(response?.result ?? response, null, 2)
      }
    } catch (error) {
      rpcErrorText = error?.message ?? String(error)
    } finally {
      rpcRunning = false
    }
  }
</script>

<div class="tool-surface connection-page">
  <section class="tool-hero">
    <div class="tool-hero-title">Connection</div>
    <div class="tool-hero-desc">
      Local transport status, in-page dispatcher access, and view JSON-RPC reference.
    </div>
  </section>

  <section class="connection-grid">
    <div class="connection-column">
      <section class="tool-section">
        <div class="tool-section-title">WEBSOCKET SERVER</div>
        <div class="ws-server-card">
          <button
            class="ws-server-strip"
            class:is-connected={IS_NODE_BACKED && $wsBridgeStatus === 'connected'}
            class:is-available={IS_NODE_BACKED && $wsBridgeStatus === 'available'}
            class:is-unavailable={!IS_NODE_BACKED || $wsBridgeStatus === 'unavailable'}
            on:click={() => IS_NODE_BACKED && navigator.clipboard?.writeText(RPC_WS_URL)}
            title={IS_NODE_BACKED ? 'Click to copy URL' : ''}
          >
            <div class="ws-strip-left">
              <span class="ws-strip-icon"><Wifi size={16} /></span>
              <div class="ws-strip-labels">
                <span class="ws-strip-title">WebSocket</span>
                <span class="ws-strip-state">
                  {wsStatusText}
                </span>
              </div>
            </div>
            <div class="ws-strip-right">
              <span class="ws-strip-url connection-mono">
                {IS_NODE_BACKED ? RPC_WS_URL : '—'}
              </span>
              {#if IS_NODE_BACKED}
                <span class="ws-copy-btn" aria-hidden="true">
                  <Copy size={16} />
                </span>
              {/if}
            </div>
          </button>
        </div>
      </section>

      <section class="tool-section">
        <div class="tool-section-title">In-Page Tester</div>
        {#if IS_NODE_BACKED}
          <div class="rpc-panel">
            <label class="rpc-field">
              <span>Method</span>
              <input bind:value={rpcMethod} type="text" spellcheck="false" placeholder="session.status" />
            </label>
            <label class="rpc-field">
              <span>Params (JSON)</span>
              <textarea bind:value={rpcParamsText} rows="6" spellcheck="false" placeholder={'{}'}></textarea>
            </label>
            <button class="rpc-run-btn" on:click={runInPageRpc} disabled={rpcRunning}>
              {rpcRunning ? 'Running…' : 'Run In-Page RPC'}
            </button>
            {#if rpcResultText}
              <label class="rpc-field">
                <span>Result</span>
                <textarea class="rpc-output" rows="8" readonly value={rpcResultText}></textarea>
              </label>
            {/if}
            {#if rpcErrorText}
              <label class="rpc-field">
                <span>Error</span>
                <textarea class="rpc-output rpc-output-error" rows="6" readonly value={rpcErrorText}></textarea>
              </label>
            {/if}
          </div>
        {:else}
          <div class="connection-empty">
            In-page testing is unavailable in static builds.
          </div>
        {/if}
      </section>

    </div>

    <div class="connection-column docs-column">
      <section class="tool-section">
        <div class="tool-section-title">API Docs</div>

        <div class="rpc-docs">
          {#each namespaces as namespace}
            <details class="rpc-namespace">
              <summary class="rpc-namespace-summary">
                <span class="rpc-namespace-name">{namespace.id}</span>
                <span class="rpc-namespace-desc">{namespace.summary}</span>
              </summary>

              <div class="rpc-handle-list">
                {#each namespace.handles as handle}
                  <details class="rpc-handle">
                    <summary class="rpc-handle-summary">
                      <span class="rpc-handle-name">{handle.name}</span>
                    </summary>
                    <div class="rpc-handle-body">
                      <div class="rpc-doc-row">
                        <span class="rpc-doc-label">Purpose</span>
                        <span class="rpc-doc-text">{handle.purpose}</span>
                      </div>
                      <div class="rpc-doc-row">
                        <span class="rpc-doc-label">Params</span>
                        <code class="rpc-doc-code">{handle.params}</code>
                      </div>
                      <div class="rpc-doc-row">
                        <span class="rpc-doc-label">Result</span>
                        <code class="rpc-doc-code">{handle.result}</code>
                      </div>
                      <div class="rpc-doc-row">
                        <span class="rpc-doc-label">Example</span>
                        <code class="rpc-doc-code">{handle.example}</code>
                      </div>
                    </div>
                  </details>
                {/each}
              </div>
            </details>
          {/each}
        </div>
      </section>
    </div>
  </section>
</div>

<style>
  .connection-page {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }
  .connection-grid {
    display: grid;
    grid-template-columns: minmax(320px, 0.95fr) minmax(420px, 1.25fr);
    gap: 12px;
  }
  .ws-server-card {
    padding: 2px 0;
  }
  .ws-server-strip {
    display: grid;
    grid-template-columns: auto 1fr;
    align-items: center;
    border-radius: 6px;
    overflow: hidden;
    border: 1px solid var(--border);
    background: rgba(255, 255, 255, 0.02);
    transition: background 0.2s, border-color 0.2s;
    width: 100%;
    text-align: left;
    font-family: inherit;
    cursor: pointer;
    padding: 0;
  }
  .ws-server-strip:hover .ws-copy-btn { opacity: 1; color: var(--accent); }
  .ws-strip-right {
    padding: 9px 12px;
    min-width: 0;
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .ws-copy-btn {
    flex-shrink: 0;
    margin-left: auto;
    display: inline-flex;
    align-items: center;
    color: var(--muted);
    padding: 2px;
    opacity: 0.45;
    transition: opacity 0.12s, color 0.12s;
  }
  .ws-strip-url {
    font-size: 0.82rem;
    color: var(--muted);
    word-break: break-all;
    flex: 1;
    min-width: 0;
  }
  .ws-strip-left {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 9px 12px;
    border-right: 1px solid var(--border);
    flex-shrink: 0;
    transition: border-color 0.2s;
  }
  .ws-strip-icon {
    display: flex;
    align-items: center;
    flex-shrink: 0;
    color: var(--muted);
    transition: color 0.2s;
  }
  .ws-strip-labels {
    display: flex;
    flex-direction: column;
    gap: 1px;
  }
  .ws-strip-title {
    font-size: 0.78rem;
    font-weight: 600;
    color: var(--text);
    text-transform: uppercase;
    letter-spacing: 0.04em;
    white-space: nowrap;
  }
  .ws-strip-state {
    font-size: 0.68rem;
    color: var(--muted);
    font-family: var(--mono);
    white-space: nowrap;
    transition: color 0.2s;
  }
  /* connected — uniform green */
  /* connected — blue */
  .ws-server-strip.is-connected {
    border-color: rgba(76, 201, 240, 0.25);
    background: rgba(76, 201, 240, 0.07);
  }
  .ws-server-strip.is-connected .ws-strip-left {
    border-right-color: rgba(76, 201, 240, 0.18);
  }
  .ws-server-strip.is-connected .ws-strip-icon,
  .ws-server-strip.is-connected .ws-strip-state {
    color: var(--accent);
  }
  /* available — green (server up, agent not yet ack'd) */
  .ws-server-strip.is-available {
    border-color: rgba(61, 214, 140, 0.25);
    background: rgba(61, 214, 140, 0.07);
  }
  .ws-server-strip.is-available .ws-strip-left {
    border-right-color: rgba(61, 214, 140, 0.18);
  }
  .ws-server-strip.is-available .ws-strip-icon,
  .ws-server-strip.is-available .ws-strip-state {
    color: #3dd68c;
  }
  /* unavailable */
  .ws-server-strip.is-unavailable {
    border-color: rgba(255, 107, 107, 0.2);
    background: rgba(255, 107, 107, 0.05);
  }
  .ws-server-strip.is-unavailable .ws-strip-left {
    border-right-color: rgba(255, 107, 107, 0.15);
  }
  .ws-server-strip.is-unavailable .ws-strip-icon,
  .ws-server-strip.is-unavailable .ws-strip-state {
    color: #ff8b8b;
  }
  @media (max-width: 640px) {
    .ws-server-strip {
      grid-template-columns: 1fr;
    }
    .ws-strip-left {
      border-right: none;
      border-bottom: 1px solid var(--border);
    }
  }
  .connection-column {
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 12px;
  }
  .docs-column .tool-section {
    height: 100%;
  }
  .connection-mono,
  .rpc-doc-code {
    font-family: var(--mono);
  }
  .connection-empty {
    padding: 12px;
    border: 1px dashed var(--border);
    border-radius: 6px;
    color: var(--muted);
    font-size: 0.82rem;
  }
  .rpc-panel {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }
  .rpc-field {
    display: flex;
    flex-direction: column;
    gap: 5px;
  }
  .rpc-field span,
  .rpc-doc-label {
    font-size: 0.68rem;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: 0.05em;
  }
  .rpc-field input,
  .rpc-field textarea {
    width: 100%;
    border: 1px solid var(--border);
    background: rgba(255, 255, 255, 0.02);
    color: var(--text);
    border-radius: 6px;
    padding: 8px 10px;
    font-family: var(--mono);
    font-size: 0.72rem;
    resize: vertical;
  }
  .rpc-run-btn {
    align-self: flex-start;
    border: 1px solid var(--accent);
    background: transparent;
    color: var(--accent);
    border-radius: 6px;
    padding: 8px 12px;
    font: inherit;
    cursor: pointer;
  }
  .rpc-run-btn:disabled {
    opacity: 0.6;
    cursor: default;
  }
  .rpc-output {
    min-height: 120px;
  }
  .rpc-output-error {
    color: #ff9a9a;
  }
  .rpc-docs {
    display: flex;
    flex-direction: column;
    gap: 10px;
    margin-top: 10px;
  }
  .rpc-namespace,
  .rpc-handle {
    border: 1px solid var(--border);
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.02);
    overflow: hidden;
  }
  .rpc-namespace-summary,
  .rpc-handle-summary {
    list-style: none;
    cursor: pointer;
  }
  .rpc-namespace-summary::-webkit-details-marker,
  .rpc-handle-summary::-webkit-details-marker {
    display: none;
  }
  .rpc-namespace-summary {
    display: flex;
    flex-direction: column;
    gap: 4px;
    padding: 12px 14px;
  }
  .rpc-namespace-name {
    font-size: 0.8rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: var(--text);
  }
  .rpc-namespace-desc {
    color: var(--muted);
    font-size: 0.8rem;
    line-height: 1.45;
  }
  .rpc-handle-list {
    display: flex;
    flex-direction: column;
    gap: 8px;
    padding: 0 10px 10px;
  }
  .rpc-handle-summary {
    padding: 10px 12px;
    background: rgba(255, 255, 255, 0.02);
  }
  .rpc-handle-name {
    font-family: var(--mono);
    color: var(--accent);
    font-size: 0.72rem;
  }
  .rpc-handle-body {
    display: flex;
    flex-direction: column;
    gap: 10px;
    padding: 0 12px 12px;
  }
  .rpc-doc-row {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }
  .rpc-doc-text {
    color: var(--text);
    font-size: 0.82rem;
    line-height: 1.45;
  }
  .rpc-doc-code {
    color: var(--text);
    font-size: 0.7rem;
    white-space: pre-wrap;
    word-break: break-word;
    background: rgba(255, 255, 255, 0.02);
    border: 1px solid var(--border);
    border-radius: 6px;
    padding: 8px 10px;
  }
  @media (max-width: 1080px) {
    .connection-grid {
      grid-template-columns: 1fr;
    }
  }
</style>
