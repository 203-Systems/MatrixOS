<script>
  import { serialEvents, serialConnected, clearSerialEvents } from '../../stores/serial.js'
  import { sendSerialText, sendSerialHex } from '../../handles/serial.js'
  import { Search, TrashCan, Information, Time } from 'carbon-icons-svelte'

  const _ansiColorMap = {
    30: '#60656f', 31: '#ff6b6b', 32: '#6bdd8b', 33: '#f7c266',
    34: '#6ba7ff', 35: '#d47fff', 36: '#5ad4ff', 37: '#e6e6ea',
    90: '#9ea1ad', 91: '#ff8b8b', 92: '#7ff7a1', 93: '#ffd27f',
    94: '#86b8ff', 95: '#e2a0ff', 96: '#8de5ff', 97: '#ffffff'
  }

  function ansiToSegments(raw) {
    if (!raw) return [{ text: raw, color: null }]
    const segments = []
    let lastIndex = 0
    let currentColor = null
    const re = /\x1B\[([0-9;]*)m/g
    let m
    while ((m = re.exec(raw)) !== null) {
      if (m.index > lastIndex) segments.push({ text: raw.slice(lastIndex, m.index), color: currentColor })
      lastIndex = m.index + m[0].length
      const parts = m[1].split(';').map(Number).filter(v => !isNaN(v))
      if (parts.length === 0 || parts.includes(0)) currentColor = null
      const code = parts.find(v => (v >= 30 && v <= 37) || (v >= 90 && v <= 97))
      if (code != null && _ansiColorMap[code]) currentColor = _ansiColorMap[code]
    }
    if (lastIndex < raw.length) segments.push({ text: raw.slice(lastIndex), color: currentColor })
    return segments.length ? segments : [{ text: raw, color: null }]
  }
  export let showHero = true
  export let onCloseHero = () => {}

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
  {#if showHero}
  <div class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">Serial</div>
    <div class="tool-hero-desc">Serial port monitor and sender. View TX/RX traffic and inject test data as text or raw hex bytes.</div>
  </div>
  {/if}
  <!-- Filter toolbar -->
  <div class="filter-bar">
    <div class="filter-search" class:filter-active={filterQuery}>
      <Search size={16} />
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
      <Information size={16} />
    </button>
    <button
      class="toolbar-toggle"
      class:toolbar-toggle-active={showTimestamps}
      on:click={() => showTimestamps = !showTimestamps}
      title="Toggle timestamp column"
    ><Time size={16} /></button>
    <button
      class="toolbar-toggle"
      class:toolbar-toggle-active={viewHex}
      on:click={() => viewHex = !viewHex}
      title="Toggle hex view"
    >HEX</button>
    <button class="clear-btn" on:click={clearSerialEvents} title="Clear serial events">
      <TrashCan size={16} />
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
            <span class="event-data">
              {#if viewHex}
                {evt.hexView}
              {:else}
                {#each ansiToSegments(evt.text) as seg}
                  <span style={seg.color ? `color:${seg.color}` : undefined}>{seg.text}</span>
                {/each}
              {/if}
            </span>
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
        placeholder={sendMode === 'hex' ? 'e.g. DE AD BE EF' : 'Type message…'}
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
    padding: 0 7px;
    height: 24px;
    box-sizing: border-box;
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
    padding: 0 6px;
    height: 24px;
    box-sizing: border-box;
    display: inline-flex;
    align-items: center;
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
    padding: 0 5px;
    height: 24px;
    box-sizing: border-box;
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
    padding: 0 8px;
    height: 24px;
    box-sizing: border-box;
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
    margin-left: auto;
    cursor: pointer;
    user-select: none;
  }
  .newline-opt input {
    appearance: none;
    -webkit-appearance: none;
    width: 12px;
    height: 12px;
    margin: 0;
    cursor: pointer;
    border: 1px solid var(--border);
    border-radius: 2px;
    background: var(--bg-2);
    flex-shrink: 0;
    position: relative;
    transition: border-color 0.1s, background 0.1s;
  }
  .newline-opt input:checked {
    background: var(--accent);
    border-color: var(--accent);
  }
  .newline-opt input:checked::after {
    content: '';
    position: absolute;
    left: 3px;
    top: 1px;
    width: 4px;
    height: 7px;
    border: 1.5px solid #0f1014;
    border-top: none;
    border-left: none;
    transform: rotate(45deg);
  }
  .sender-row { display: flex; gap: 6px; }
  .sender-input {
    flex: 1;
    background: var(--bg-2);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--text);
    font-family: var(--mono);
    font-size: 0.76rem;
    padding: 0 8px;
    height: 24px;
    box-sizing: border-box;
    outline: none;
  }
  .sender-input::placeholder { color: var(--muted); opacity: 0.6; }
  .sender-input:focus { border-color: var(--accent); }
  .send-btn {
    background: rgba(76, 201, 240, 0.12);
    border: 1px solid var(--accent);
    border-radius: 6px;
    color: var(--accent);
    font: inherit;
    font-size: 0.82rem;
    padding: 0 8px;
    height: 24px;
    box-sizing: border-box;
    cursor: pointer;
    transition: background 0.12s, opacity 0.12s;
    white-space: nowrap;
  }
  .send-btn:not(:disabled):hover { background: rgba(76, 201, 240, 0.2); }
  .send-btn:disabled { opacity: 0.4; cursor: not-allowed; }
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
