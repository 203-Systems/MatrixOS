<script>
  import { hidEvents, hidConnected, clearHidEvents, sendRawHid } from '../../stores/hid.js'
  import { Search, Information, TrashCan, Time } from 'carbon-icons-svelte'

  let eventBody
  let autoScroll = true
  let hexInput = ''
  let filterQuery = ''
  let showFilterHelp = false
  let showTimestamps = false

  const catColors = { Keyboard: 'cat-kb', Gamepad: 'cat-gp', RawHID: 'cat-raw' }

  // Auto-scroll on new events
  $: if ($hidEvents && autoScroll && eventBody) {
    requestAnimationFrame(() => {
      if (eventBody) eventBody.scrollTop = eventBody.scrollHeight
    })
  }

  // Filter — supports dir:, cat:, plain text
  $: filteredEvents = (() => {
    const q = filterQuery.trim().toLowerCase()
    if (!q) return $hidEvents
    const dirM = q.match(/\bdir:(tx|rx)\b/)
    const catM = q.match(/\bcat:(\S+)/)
    const plain = q.replace(/\b(dir|cat):\S+/g, '').trim()
    return $hidEvents.filter(evt => {
      if (dirM && evt.direction.toLowerCase() !== dirM[1]) return false
      if (catM && !evt.category.toLowerCase().includes(catM[1])) return false
      if (plain) return evt.summary.toLowerCase().includes(plain) || evt.category.toLowerCase().includes(plain) || evt.direction.toLowerCase().includes(plain)
      return true
    })
  })()

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
  <!-- Filter toolbar -->
  <div class="filter-bar">
    <div class="filter-search" class:filter-active={filterQuery}>
      <Search size={13} />
      <input
        type="text"
        class="filter-input"
        placeholder="Filter HID…"
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
      title="Toggle timestamps"
    >
      <Time size={13} />
    </button>
    <button class="clear-btn" on:click={clearHidEvents} title="Clear HID events">
      <TrashCan size={13} />
    </button>
  </div>

  {#if showFilterHelp}
    <div class="filter-help">
      <span><code>dir:TX</code> or <code>dir:RX</code> — direction</span>
      <span><code>cat:Keyboard</code> — category (Keyboard, Gamepad, RawHID)</span>
      <span>Plain text matches summary and category.</span>
    </div>
  {/if}

  <!-- Event log -->
  <div class="event-section">
    <div class="event-col-header" class:no-time={!showTimestamps}>
      {#if showTimestamps}<span class="col-time">Time</span>{/if}
      <span class="col-cat">Category</span>
      <span class="col-sum">Summary</span>
    </div>
    <div class="event-body" bind:this={eventBody} on:scroll={handleScroll}>
      {#if filteredEvents.length === 0}
        <div class="empty-msg">{$hidEvents.length === 0 ? 'No HID events captured yet.' : 'No events match the filter.'}</div>
      {:else}
        {#each filteredEvents as evt (evt.id)}
          <div class="event-row" class:no-time={!showTimestamps}>
            {#if showTimestamps}<span class="event-time col-time">{evt.timestamp}</span>{/if}
            <span class="event-cat {catColors[evt.category] || ''}">
              {evt.category === 'RawHID' ? (evt.direction === 'TX' ? 'Raw Out' : 'Raw In') : evt.category}
            </span>
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
  .filter-count {
    font-size: 0.68rem;
    color: var(--muted);
    font-family: var(--mono);
    white-space: nowrap;
  }
  .toolbar-toggle {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 6px;
    font-size: 0.66rem;
    display: inline-flex;
    align-items: center;
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
  .event-time { color: var(--muted); }
  .event-cat { min-width: 56px; flex-shrink: 0; font-weight: 600; }
  .cat-kb { color: var(--accent); }
  .cat-gp { color: #c49bff; }
  .cat-raw { color: #f0a04b; }
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
  .sender-row { display: flex; gap: 6px; align-items: center; }
  .hex-input {
    flex: 1;
    font-family: var(--mono);
    font-size: 0.72rem;
    background: var(--bg-2);
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
    background: transparent;
    color: var(--accent);
    border: 1px solid var(--accent);
    border-radius: 4px;
    padding: 4px 10px;
    cursor: pointer;
    font-weight: 600;
    flex-shrink: 0;
  }
  .sender-btn:hover { background: rgba(76, 201, 240, 0.12); }
  .sender-hint {
    display: block;
    font-size: 0.65rem;
    color: var(--muted);
    opacity: 0.6;
    margin-top: 3px;
    font-family: var(--mono);
  }

  .event-col-header {
    display: flex;
    gap: 6px;
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
  .col-time { min-width: 80px; flex-shrink: 0; }
  .col-cat  { min-width: 56px; flex-shrink: 0; }
  .col-sum  { flex: 1; min-width: 0; }
  .no-time .col-time { display: none; }
</style>