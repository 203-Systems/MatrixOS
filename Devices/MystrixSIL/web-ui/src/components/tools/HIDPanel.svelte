<script>
  import { hidEvents, hidConnected, clearHidEvents, sendRawHid } from '../../stores/hid.js'
  import { activeGridKeys, fnKeyActive, runtimeGridKeys, runtimeFnActive } from '../../stores/input.js'
  import { browserCapabilities } from '../../stores/tooling.js'
  import { Close } from 'carbon-icons-svelte'

  let eventBody
  let autoScroll = true
  let hexInput = ''

  $: hidTapTone = $hidConnected ? 'live' : 'idle'
  $: hidTapLabel = $hidConnected ? 'Connected' : 'Not connected'

  $: webHidLabel = $browserCapabilities.detected
    ? ($browserCapabilities.hid ? 'Available' : 'Unavailable')
    : 'Detecting'
  $: webHidTone = !$browserCapabilities.detected ? 'idle'
    : $browserCapabilities.hid ? 'live' : 'warn'

  $: gamepadLabel = $browserCapabilities.detected
    ? ($browserCapabilities.gamepad ? 'Available' : 'Unavailable')
    : 'Detecting'
  $: gamepadTone = !$browserCapabilities.detected ? 'idle'
    : $browserCapabilities.gamepad ? 'live' : 'warn'

  // Category color mapping
  const catColors = { Keyboard: 'cat-kb', Gamepad: 'cat-gp', RawHID: 'cat-raw' }

  // Auto-scroll on new events
  $: if ($hidEvents && autoScroll && eventBody) {
    requestAnimationFrame(() => {
      if (eventBody) eventBody.scrollTop = eventBody.scrollHeight
    })
  }

  function handleScroll() {
    if (!eventBody) return
    autoScroll = eventBody.scrollTop + eventBody.clientHeight >= eventBody.scrollHeight - 16
  }

  function parseHex(str) {
    const tokens = str.trim().split(/[\s,]+/)
    const bytes = []
    for (const t of tokens) {
      const v = parseInt(t, 16)
      if (isNaN(v) || v < 0 || v > 255) return null
      bytes.push(v)
    }
    return bytes.length > 0 ? bytes : null
  }

  function sendHex() {
    const bytes = parseHex(hexInput)
    if (!bytes) return
    if (bytes.length > 32) return
    sendRawHid(bytes)
    hexInput = ''
  }

  function handleHexKeydown(e) {
    if (e.key === 'Enter') sendHex()
  }
</script>

<div class="hid-panel">
  <!-- Status bar -->
  <div class="status-bar">
    <span class="status-item">
      HID tap: <span class="tool-value-{hidTapTone}">{hidTapLabel}</span>
    </span>
    <span class="status-sep">·</span>
    <span class="status-item">
      WebHID: <span class="tool-value-{webHidTone}">{webHidLabel}</span>
    </span>
    <span class="status-sep">·</span>
    <span class="status-item">
      Gamepad: <span class="tool-value-{gamepadTone}">{gamepadLabel}</span>
    </span>
  </div>

  <!-- Event log -->
  <div class="event-section">
    <div class="section-header">
      <span class="section-title">HID Traffic</span>
      <span class="section-count">{$hidEvents.length}</span>
      <button class="section-action" on:click={clearHidEvents} title="Clear events">
        <Close size={14} />
      </button>
    </div>
    <div
      class="event-body"
      bind:this={eventBody}
      on:scroll={handleScroll}
    >
      {#if $hidEvents.length === 0}
        <div class="empty-msg">No HID events captured yet.</div>
      {:else}
        {#each $hidEvents as evt (evt.id)}
          <div class="event-row">
            <span class="event-time">{evt.timestamp}</span>
            <span class="event-cat {catColors[evt.category] || ''}">{evt.category}</span>
            <span class="event-dir" class:dir-tx={evt.direction === 'TX'} class:dir-rx={evt.direction === 'RX'}>{evt.direction}</span>
            <span class="event-summary">{evt.summary}</span>
            {#if evt.rawPayload}
              <span class="event-raw">{evt.rawPayload}</span>
            {/if}
          </div>
        {/each}
      {/if}
    </div>
  </div>

  <!-- Raw HID injection -->
  <div class="sender-section">
    <div class="sender-row">
      <input
        type="text"
        class="hex-input"
        bind:value={hexInput}
        on:keydown={handleHexKeydown}
        placeholder="Hex bytes: 01 02 03 FF"
        spellcheck="false"
      />
      <button class="sender-btn" on:click={sendHex} title="Send raw HID bytes">Send</button>
    </div>
    <span class="sender-hint">Max 32 bytes, space-separated hex</span>
  </div>
</div>

<style>
  .hid-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    gap: 0;
    padding: 10px 14px;
    overflow: hidden;
  }

  /* Status bar */
  .status-bar {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 0.72rem;
    font-family: var(--mono);
    color: var(--muted);
    padding-bottom: 8px;
    border-bottom: 1px solid var(--border);
    flex-shrink: 0;
    flex-wrap: wrap;
  }
  .status-sep { opacity: 0.3; }

  /* Event log */
  .event-section {
    flex: 1;
    display: flex;
    flex-direction: column;
    min-height: 0;
    padding-top: 8px;
  }
  .section-header {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 6px;
    flex-shrink: 0;
  }
  .section-title {
    font-weight: 600;
    font-size: 0.82rem;
    letter-spacing: 0.03em;
    text-transform: uppercase;
    color: var(--text);
  }
  .section-count {
    font-size: 0.72rem;
    color: var(--muted);
    font-family: var(--mono);
  }
  .section-action {
    margin-left: auto;
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 4px;
    display: inline-flex;
    align-items: center;
  }
  .section-action:hover { color: var(--text); border-color: var(--accent); }

  .event-body {
    flex: 1;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    gap: 1px;
    font-family: var(--mono);
    font-size: 0.72rem;
    scrollbar-width: thin;
    scrollbar-color: rgba(255,255,255,0.12) transparent;
  }

  .event-row {
    display: flex;
    gap: 6px;
    padding: 2px 6px;
    border-radius: 3px;
    align-items: baseline;
    white-space: nowrap;
    background: rgba(255, 255, 255, 0.015);
  }

  .event-time { color: var(--muted); min-width: 80px; flex-shrink: 0; }

  .event-cat {
    min-width: 56px;
    flex-shrink: 0;
    font-weight: 600;
  }
  .cat-kb { color: var(--accent); }
  .cat-gp { color: #c49bff; }
  .cat-raw { color: #f0a04b; }

  .event-dir { min-width: 20px; flex-shrink: 0; font-weight: 600; }
  .dir-tx { color: var(--accent); }
  .dir-rx { color: #6bdd8b; }

  .event-summary { color: var(--text); flex: 1; min-width: 0; overflow: hidden; text-overflow: ellipsis; }
  .event-raw { color: var(--muted); opacity: 0.5; flex-shrink: 0; }

  .empty-msg {
    color: var(--muted);
    font-size: 0.82rem;
    text-align: center;
    padding: 24px;
    font-family: var(--sans);
  }

  /* Sender section */
  .sender-section {
    flex-shrink: 0;
    border-top: 1px solid var(--border);
    padding-top: 8px;
    margin-top: 4px;
  }
  .sender-row {
    display: flex;
    gap: 6px;
    align-items: center;
  }
  .hex-input {
    flex: 1;
    font-family: var(--mono);
    font-size: 0.72rem;
    background: var(--bg);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 4px 8px;
    letter-spacing: 0.05em;
  }
  .hex-input::placeholder { color: var(--muted); opacity: 0.5; }
  .hex-input:focus { border-color: var(--accent); outline: none; }
  .sender-btn {
    font-family: var(--mono);
    font-size: 0.72rem;
    background: var(--accent);
    color: var(--bg);
    border: none;
    border-radius: 4px;
    padding: 4px 10px;
    cursor: pointer;
    font-weight: 600;
    flex-shrink: 0;
  }
  .sender-btn:hover { opacity: 0.85; }
  .sender-hint {
    display: block;
    font-size: 0.65rem;
    color: var(--muted);
    opacity: 0.6;
    margin-top: 3px;
    font-family: var(--mono);
  }
</style>
