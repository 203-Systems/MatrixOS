<script>
  import { serialEvents, serialConnected, clearSerialEvents, sendSerialText, sendSerialHex } from '../../stores/serial.js'
  import { Close } from 'carbon-icons-svelte'

  let eventBody
  let autoScroll = true
  let viewHex = false
  let sendMode = 'text'
  let sendValue = ''
  let appendNewline = true
  let hexError = ''

  $: if ($serialEvents && autoScroll && eventBody) {
    requestAnimationFrame(() => {
      if (eventBody) eventBody.scrollTop = eventBody.scrollHeight
    })
  }

  function handleSend() {
    if (!sendValue.trim()) return
    if (sendMode === 'hex') {
      if (!/^[\da-fA-F\s]+$/.test(sendValue.trim())) {
        hexError = 'Invalid hex characters'
        return
      }
      const tokens = sendValue.trim().split(/\s+/)
      if (tokens.some(t => t.length > 2)) {
        hexError = 'Each byte must be 2 hex digits'
        return
      }
      hexError = ''
      sendSerialHex(sendValue)
    } else {
      const text = appendNewline ? sendValue + '\n' : sendValue
      sendSerialText(text)
    }
    sendValue = ''
  }

  function handleKeydown(e) {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault()
      handleSend()
    }
  }
</script>

<div class="serial-panel">
  <!-- Status bar -->
  <div class="status-bar">
    <span class="status-pill" class:status-live={$serialConnected} class:status-idle={!$serialConnected}>
      {$serialConnected ? 'Connected' : 'Waiting'}
    </span>
    <span class="status-count">{$serialEvents.length} events</span>
  </div>

  <!-- Event log -->
  <div class="event-section">
    <div class="section-header">
      <span class="section-title">Serial Traffic</span>
      <span class="section-count">{$serialEvents.length}</span>
      <button
        class="view-toggle"
        class:active={viewHex}
        on:click={() => viewHex = !viewHex}
        title="Toggle hex view"
      >HEX</button>
      <button class="section-action" on:click={clearSerialEvents} title="Clear events">
        <Close size={14} />
      </button>
    </div>
    <div
      class="event-body"
      bind:this={eventBody}
      on:scroll={() => {
        if (!eventBody) return
        autoScroll = eventBody.scrollTop + eventBody.clientHeight >= eventBody.scrollHeight - 16
      }}
    >
      {#if $serialEvents.length === 0}
        <div class="empty-msg">No serial data captured yet.</div>
      {:else}
        {#each $serialEvents as evt (evt.id)}
          <div class="event-row" class:row-tx={evt.direction === 'TX'}>
            <span class="event-time">{evt.timestamp}</span>
            <span class="event-dir" class:dir-tx={evt.direction === 'TX'} class:dir-rx={evt.direction === 'RX'}>
              {evt.direction}
            </span>
            <span class="event-data">{viewHex ? evt.hexView : evt.text}</span>
          </div>
        {/each}
      {/if}
    </div>
  </div>

  <!-- Sender -->
  <div class="sender">
    <div class="sender-controls">
      <button
        class="mode-btn"
        class:active={sendMode === 'text'}
        on:click={() => { sendMode = 'text'; hexError = '' }}
      >Text</button>
      <button
        class="mode-btn"
        class:active={sendMode === 'hex'}
        on:click={() => { sendMode = 'hex'; hexError = '' }}
      >Hex</button>
      {#if sendMode === 'text'}
        <label class="newline-opt">
          <input type="checkbox" bind:checked={appendNewline} />
          <span>\\n</span>
        </label>
      {/if}
    </div>
    <div class="sender-row">
      <input
        class="sender-input"
        type="text"
        placeholder={sendMode === 'hex' ? '48 65 6C 6C 6F' : 'Type message…'}
        bind:value={sendValue}
        on:keydown={handleKeydown}
      />
      <button class="send-btn" on:click={handleSend} disabled={!sendValue.trim()}>Send</button>
    </div>
    {#if hexError}
      <span class="hex-error">{hexError}</span>
    {/if}
  </div>
</div>

<style>
  .serial-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    gap: 10px;
    padding: 14px;
    overflow: hidden;
  }

  /* Status bar */
  .status-bar {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-shrink: 0;
  }
  .status-count {
    font-size: 0.72rem;
    color: var(--muted);
    font-family: var(--mono);
  }

  /* Section header */
  .section-header {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 6px;
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
  .view-toggle {
    margin-left: auto;
    background: rgba(255, 255, 255, 0.04);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 1px 6px;
    font-size: 0.66rem;
    font-family: var(--mono);
    font-weight: 600;
    letter-spacing: 0.04em;
  }
  .view-toggle:hover { color: var(--text); border-color: var(--accent); }
  .view-toggle.active {
    color: var(--accent);
    border-color: rgba(76, 201, 240, 0.3);
    background: rgba(76, 201, 240, 0.08);
  }

  /* Event log */
  .event-section {
    flex: 1;
    display: flex;
    flex-direction: column;
    min-height: 0;
  }
  .event-body {
    flex: 1;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    gap: 1px;
    font-family: var(--mono);
    font-size: 0.73rem;
    scrollbar-width: thin;
    scrollbar-color: rgba(255,255,255,0.12) transparent;
  }
  .event-row {
    display: flex;
    gap: 8px;
    padding: 3px 6px;
    border-radius: 3px;
    background: rgba(255, 255, 255, 0.015);
    align-items: baseline;
  }
  .row-tx { background: rgba(76, 201, 240, 0.04); }
  .event-time { color: var(--muted); min-width: 85px; flex-shrink: 0; }
  .event-dir {
    min-width: 24px;
    font-weight: 600;
    flex-shrink: 0;
  }
  .dir-tx { color: var(--accent); }
  .dir-rx { color: #6bdd8b; }
  .event-data { word-break: break-all; color: var(--text); }
  .empty-msg {
    color: var(--muted);
    font-size: 0.82rem;
    text-align: center;
    padding: 24px;
    font-family: var(--sans);
  }

  /* Sender */
  .sender {
    flex-shrink: 0;
    display: flex;
    flex-direction: column;
    gap: 6px;
    border-top: 1px solid var(--border);
    padding-top: 10px;
  }
  .sender-controls {
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .mode-btn {
    background: rgba(255, 255, 255, 0.04);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 8px;
    font-size: 0.7rem;
    font-weight: 600;
  }
  .mode-btn:hover { color: var(--text); }
  .mode-btn.active {
    color: var(--accent);
    border-color: rgba(76, 201, 240, 0.3);
    background: rgba(76, 201, 240, 0.08);
  }
  .newline-opt {
    display: flex;
    align-items: center;
    gap: 4px;
    font-size: 0.68rem;
    color: var(--muted);
    font-family: var(--mono);
    margin-left: 4px;
    cursor: pointer;
  }
  .newline-opt input { width: 12px; height: 12px; margin: 0; cursor: pointer; }
  .sender-row {
    display: flex;
    gap: 6px;
  }
  .sender-input {
    flex: 1;
    background: rgba(255, 255, 255, 0.04);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--text);
    font-family: var(--mono);
    font-size: 0.76rem;
    padding: 5px 8px;
    outline: none;
  }
  .sender-input::placeholder { color: var(--muted); opacity: 0.6; }
  .sender-input:focus { border-color: var(--accent); }
  .send-btn {
    background: rgba(76, 201, 240, 0.12);
    border: 1px solid rgba(76, 201, 240, 0.25);
    border-radius: 4px;
    color: var(--accent);
    font-size: 0.74rem;
    font-weight: 600;
    padding: 5px 14px;
    cursor: pointer;
  }
  .send-btn:hover { background: rgba(76, 201, 240, 0.2); }
  .send-btn:disabled { opacity: 0.4; cursor: default; }
  .hex-error {
    font-size: 0.68rem;
    color: var(--danger);
    font-family: var(--mono);
  }
</style>
