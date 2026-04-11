<script>
  import { serialEvents, serialConnected, clearSerialEvents, sendSerialText, sendSerialHex } from '../../stores/serial.js'
  import { Search, TrashCan, Information, Time } from 'carbon-icons-svelte'

  let eventBody
  let autoScroll = true
  let viewHex = false
  let sendMode = 'text'
  let sendValue = ''
  let appendNewline = true
  let hexError = ''
  let filterQuery = ''
  let showTimestamps = false
  let showFilterHelp = false

  $: if ($serialEvents && autoScroll && eventBody) {
    requestAnimationFrame(() => {
      if (eventBody) eventBody.scrollTop = eventBody.scrollHeight
    })
  }

  $: filteredEvents = (() => {
    const q = filterQuery.trim().toLowerCase()
    if (!q) return $serialEvents
    const dirM = q.match(/\bdir:(tx|rx)\b/)
    const plain = q.replace(/\bdir:\S+/g, '').trim()
    return $serialEvents.filter(evt => {
      if (dirM && evt.direction.toLowerCase() !== dirM[1]) return false
      if (plain) return (viewHex ? evt.hexView : evt.text).toLowerCase().includes(plain) || evt.direction.toLowerCase().includes(plain)
      return true
    })
  })()

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
  <!-- Filter toolbar -->
  <div class="filter-bar">
    <div class="filter-search" class:filter-active={filterQuery}>
      <Search size={13} />
      <input
        type="text"
        class="filter-input"
        placeholder="Filter… (dir:TX)"
        bind:value={filterQuery}
        spellcheck="false"
      />
    </div>
    <button
      class="toolbar-toggle"
      class:toolbar-toggle-active={showFilterHelp}
      on:click={() => showFilterHelp = !showFilterHelp}
      title="Filter syntax help"
    >
      <Information size={14} />
    </button>
    <button
      class="toolbar-toggle"
      class:toolbar-toggle-active={showTimestamps}
      on:click={() => showTimestamps = !showTimestamps}
      title="Toggle timestamp column"
    ><Time size={13} /></button>
    <button
      class="toolbar-toggle"
      class:toolbar-toggle-active={viewHex}
      on:click={() => viewHex = !viewHex}
      title="Toggle hex view"
    >HEX</button>
    <button class="clear-btn" on:click={clearSerialEvents} title="Clear serial events">
      <TrashCan size={13} />
    </button>
  </div>

  {#if showFilterHelp}
    <div class="filter-help">
      <span><code>dir:TX</code> or <code>dir:RX</code> — direction</span>
      <span>Plain text matches data content.</span>
    </div>
  {/if}

  <!-- Event log -->
  <div class="event-section">
    <div class="event-col-header" class:no-time={!showTimestamps}>
      {#if showTimestamps}<span class="col-time">Time</span>{/if}
      <span class="col-dir">Dir</span>
      <span class="col-data">Data</span>
    </div>
    <div
      class="event-body"
      bind:this={eventBody}
      on:scroll={() => {
        if (!eventBody) return
        autoScroll = eventBody.scrollTop + eventBody.clientHeight >= eventBody.scrollHeight - 16
      }}
    >
      {#if filteredEvents.length === 0}
        <div class="empty-msg">{$serialEvents.length === 0 ? 'No serial data captured yet.' : 'No events match the filter.'}</div>
      {:else}
        {#each filteredEvents as evt (evt.id)}
          <div class="event-row" class:row-tx={evt.direction === 'TX'}>
            {#if showTimestamps}<span class="event-time">{evt.timestamp}</span>{/if}
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
      <button class="mode-btn" class:active={sendMode === 'text'} on:click={() => { sendMode = 'text'; hexError = '' }}>Text</button>
      <button class="mode-btn" class:active={sendMode === 'hex'} on:click={() => { sendMode = 'hex'; hexError = '' }}>Hex</button>
      {#if sendMode === 'text'}
        <label class="newline-opt">
          <input type="checkbox" bind:checked={appendNewline} />
          <span>\n</span>
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
    gap: 0;
    padding: 10px 14px;
    overflow: hidden;
  }

  /* Filter bar */
  .filter-bar {
    display: flex;
    align-items: center;
    gap: 6px;
    padding-bottom: 8px;
    border-bottom: 1px solid var(--border);
    flex-shrink: 0;
  }
  .filter-search {
    display: flex;
    align-items: center;
    gap: 5px;
    flex: 1;
    min-width: 0;
    padding: 3px 7px;
    border: 1px solid var(--border);
    border-radius: 4px;
    background: var(--bg-2);
    color: var(--muted);
  }
  .filter-search:focus-within,
  .filter-active { border-color: rgba(76, 201, 240, 0.35); }
  .filter-input {
    flex: 1;
    background: none;
    border: none;
    outline: none;
    color: var(--text);
    font-family: var(--mono);
    font-size: 0.72rem;
    min-width: 0;
  }
  .filter-input::placeholder { color: var(--muted); }
  .toolbar-toggle {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 6px;
    font-size: 0.66rem;
    font-family: var(--mono);
    text-transform: uppercase;
    letter-spacing: 0.03em;
  }
  .toolbar-toggle:hover { color: var(--text); border-color: var(--accent); }
  .toolbar-toggle-active { color: var(--accent); border-color: rgba(76, 201, 240, 0.4); }
  .clear-btn {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 5px;
    display: inline-flex;
    align-items: center;
  }
  .clear-btn:hover { color: var(--danger); border-color: var(--danger); }

  /* Event log */
  .event-section {
    flex: 1;
    display: flex;
    flex-direction: column;
    min-height: 0;
    padding-top: 6px;
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
  .event-time { color: var(--muted); }
  .event-dir { min-width: 24px; font-weight: 600; flex-shrink: 0; }
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
    margin-top: 4px;
  }
  .sender-controls { display: flex; align-items: center; gap: 6px; }
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
  .mode-btn.active { color: var(--accent); border-color: rgba(76, 201, 240, 0.3); background: rgba(76, 201, 240, 0.08); }
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
  .sender-row { display: flex; gap: 6px; }
  .sender-input {
    flex: 1;
    background: var(--bg-2);
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
    background: transparent;
    border: 1px solid var(--accent);
    border-radius: 4px;
    color: var(--accent);
    font-size: 0.74rem;
    font-weight: 600;
    padding: 5px 14px;
    cursor: pointer;
  }
  .send-btn:hover { background: rgba(76, 201, 240, 0.12); }
  .send-btn:disabled { opacity: 0.4; cursor: default; }
  .hex-error { font-size: 0.68rem; color: var(--danger); font-family: var(--mono); }

  .event-col-header {
    display: flex;
    gap: 8px;
    padding: 2px 6px 4px;
    font-size: 0.64rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--muted);
    opacity: 0.65;
    border-bottom: 1px solid var(--border);
    flex-shrink: 0;
  }
  .col-time { min-width: 85px; flex-shrink: 0; }
  .col-dir  { min-width: 24px; flex-shrink: 0; }
  .col-data { flex: 1; min-width: 0; }
  .filter-help {
    display: flex;
    flex-wrap: wrap;
    gap: 6px 16px;
    padding: 6px 8px;
    border-radius: 4px;
    background: rgba(255, 255, 255, 0.02);
    border: 1px solid var(--border);
    font-size: 0.68rem;
    color: var(--muted);
    font-family: var(--mono);
    flex-shrink: 0;
    margin-bottom: 4px;
  }
  .filter-help code { color: var(--accent); background: rgba(76, 201, 240, 0.08); padding: 0 3px; border-radius: 2px; }
</style>