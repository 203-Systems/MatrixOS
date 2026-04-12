<script>
  import { midiEvents, midiConnected, midiPorts, clearMidiEvents, portLabel } from '../../stores/midi.js'
  import { sendMidiNote, sendMidiCC, sendMidiProgramChange } from '../../handles/midi.js'
  import { Search, Information, TrashCan, Time } from 'carbon-icons-svelte'

  let eventBody
  let autoScroll = true
  let showTimestamps = false
  let filterQuery = ''
  let showFilterHelp = false
  let showRaw = false

  // Sender state
  let msgType = 'noteOn'
  let channel = 1
  let note = 60
  let velocity = 100
  let controller = 0
  let ccValue = 64
  let program = 0
  let srcPortChoice = String(0xF000)   // MatrixOS as source by default
  let dstPortChoice = String(0x0100)   // USB MIDI as destination by default
  let srcCustomHex = ''
  let dstCustomHex = ''

  function resolvePort(choice, customHex) {
    if (choice === 'custom') {
      const n = parseInt(customHex.replace(/^0x/i, ''), 16)
      return isNaN(n) ? 0 : n
    }
    return parseInt(choice)
  }

  // Auto-scroll on new events
  $: if ($midiEvents && autoScroll && eventBody) {
    requestAnimationFrame(() => {
      if (eventBody) eventBody.scrollTop = eventBody.scrollHeight
    })
  }

  // Filter logic — supports plain text and tokens: dir:, port:, ch:, type:
  $: filteredEvents = (() => {
    const q = filterQuery.trim().toLowerCase()
    if (!q) return $midiEvents
    const dirM = q.match(/\bdir:(tx|rx)\b/)
    const portM = q.match(/\bport:(\S+)/)
    const chM = q.match(/\bch:(\d+)/)
    const typeM = q.match(/\btype:(\S+)/)
    const hasToken = dirM || portM || chM || typeM
    // Plain text (strip tokens)
    const plain = q.replace(/\b(dir|port|ch|type):\S+/g, '').trim()
    return $midiEvents.filter(evt => {
      if (dirM && evt.direction.toLowerCase() !== dirM[1]) return false
      if (portM && !evt.srcPort.toLowerCase().includes(portM[1]) && !evt.dstPort.toLowerCase().includes(portM[1])) return false
      if (chM && evt.channel !== parseInt(chM[1])) return false
      if (typeM && !evt.msgType.toLowerCase().includes(typeM[1])) return false
      if (plain) {
        return (
          evt.direction.toLowerCase().includes(plain) ||
          evt.srcPort.toLowerCase().includes(plain) ||
          evt.dstPort.toLowerCase().includes(plain) ||
          evt.msgType.toLowerCase().includes(plain) ||
          evt.summary.toLowerCase().includes(plain) ||
          (evt.channel && `ch${evt.channel}`.includes(plain))
        )
      }
      return true
    })
  })()

  function handleScroll() {
    if (!eventBody) return
    autoScroll = eventBody.scrollTop + eventBody.clientHeight >= eventBody.scrollHeight - 16
  }

  function clamp(val, min, max) {
    const n = parseInt(val, 10)
    if (isNaN(n)) return min
    return Math.max(min, Math.min(max, n))
  }

  function send() {
    const ch = clamp(channel, 1, 16) - 1
    const port = resolvePort(dstPortChoice, dstCustomHex)
    if (msgType === 'noteOn') {
      sendMidiNote(ch, clamp(note, 0, 127), clamp(velocity, 0, 127), port)
    } else if (msgType === 'noteOff') {
      sendMidiNote(ch, clamp(note, 0, 127), 0, port)
    } else if (msgType === 'cc') {
      sendMidiCC(ch, clamp(controller, 0, 127), clamp(ccValue, 0, 127), port)
    } else if (msgType === 'program') {
      sendMidiProgramChange(ch, clamp(program, 0, 127), port)
    }
  }
</script>

<div class="midi-panel">
  <!-- Filter toolbar -->
  <div class="filter-bar">
    <div class="filter-search" class:filter-active={filterQuery}>
      <Search size={13} />
      <input
        type="text"
        class="filter-input"
        placeholder="Filter MIDI…"
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
      class:toolbar-toggle-active={showRaw}
      on:click={() => showRaw = !showRaw}
      title="Toggle raw bytes column"
    >RAW</button>
    <button class="clear-btn" on:click={clearMidiEvents} title="Clear MIDI traffic">
      <TrashCan size={13} />
    </button>
  </div>

  {#if showFilterHelp}
    <div class="filter-help">
      <span><code>dir:TX</code> or <code>dir:RX</code> — direction</span>
      <span><code>port:USB</code> — port name substring</span>
      <span><code>ch:1</code> — MIDI channel</span>
      <span><code>type:Note</code> — message type substring</span>
      <span>Plain text matches any field.</span>
    </div>
  {/if}

  <!-- Event log -->
  <div class="event-section">
    <div class="event-col-header" class:no-time={!showTimestamps}>
      {#if showTimestamps}<span class="col-time">Time</span>{/if}
      <span class="col-ports">Ports</span>
      <span class="col-ch">Ch</span>
      <span class="col-type">Type</span>
      <span class="col-summary">Summary</span>
      {#if showRaw}<span class="col-raw">Raw</span>{/if}
    </div>
    <div class="event-body" bind:this={eventBody} on:scroll={handleScroll}>
      {#if filteredEvents.length === 0}
        <div class="empty-msg">{$midiEvents.length === 0 ? 'No MIDI traffic captured yet.' : 'No events match the filter.'}</div>
      {:else}
        {#each filteredEvents as evt (evt.id)}
          <div class="event-row" class:row-tx={evt.direction === 'TX'} class:row-rx={evt.direction === 'RX'} class:no-time={!showTimestamps}>
            {#if showTimestamps}<span class="col-time event-time">{evt.timestamp}</span>{/if}
            <span class="col-ports event-ports" title="{evt.srcPortHex} → {evt.dstPortHex}">{evt.srcPortLabel || evt.srcPort} → {evt.dstPortLabel || evt.dstPort}</span>
            <span class="col-ch event-ch">{evt.channel != null ? evt.channel : '—'}</span>
            <span class="col-type event-type">{evt.msgType}</span>
            <span class="col-summary event-summary">{evt.summary}</span>
            {#if showRaw}<span class="col-raw event-raw">{evt.rawBytes}</span>{/if}
          </div>
        {/each}
      {/if}
    </div>
  </div>

  <!-- Sender -->
  <div class="sender-section">
    <!-- Row 1: Port routing -->
    <div class="sender-route-row">
      <label class="sender-field">
        <span>Src</span>
        <select bind:value={srcPortChoice} class="sender-select">
          <optgroup label="Device Ports">
            {#each $midiPorts as port}
              <option value={String(port.id)}>{port.name}</option>
            {/each}
          </optgroup>
          <optgroup label="Other">
            <option value="custom">Custom…</option>
          </optgroup>
        </select>
        {#if srcPortChoice === 'custom'}
          <input
            type="text"
            class="custom-port-input"
            bind:value={srcCustomHex}
            placeholder="0x0100"
            spellcheck="false"
          />
        {/if}
      </label>
      <span class="route-arrow">→</span>
      <label class="sender-field">
        <span>Dst</span>
        <select bind:value={dstPortChoice} class="sender-select">
          <optgroup label="Device Ports">
            {#each $midiPorts as port}
              <option value={String(port.id)}>{port.name}</option>
            {/each}
          </optgroup>
          <optgroup label="Routing">
            <option value={String(0x0001)}>All Ports</option>
            <option value={String(0x0000)}>Each Class</option>
            <option value="custom">Custom…</option>
          </optgroup>
        </select>
        {#if dstPortChoice === 'custom'}
          <input
            type="text"
            class="custom-port-input"
            bind:value={dstCustomHex}
            placeholder="0x0100"
            spellcheck="false"
          />
        {/if}
      </label>
    </div>
    <!-- Row 2: MIDI message -->
    <div class="sender-msg-row">
      <label class="sender-field">
        <span>Ch</span>
        <input type="number" bind:value={channel} min="1" max="16" />
      </label>
      <label class="sender-field">
        <span>Cmd</span>
        <select bind:value={msgType} class="sender-select">
          <option value="noteOn">Note On</option>
          <option value="noteOff">Note Off</option>
          <option value="cc">Control Change</option>
          <option value="program">Program Change</option>
        </select>
      </label>
      {#if msgType === 'noteOn' || msgType === 'noteOff'}
        <label class="sender-field">
          <span>Note</span>
          <input type="number" bind:value={note} min="0" max="127" />
        </label>
        {#if msgType === 'noteOn'}
          <label class="sender-field">
            <span>Vel</span>
            <input type="number" bind:value={velocity} min="0" max="127" />
          </label>
        {/if}
      {:else if msgType === 'cc'}
        <label class="sender-field">
          <span>CC#</span>
          <input type="number" bind:value={controller} min="0" max="127" />
        </label>
        <label class="sender-field">
          <span>Val</span>
          <input type="number" bind:value={ccValue} min="0" max="127" />
        </label>
      {:else if msgType === 'program'}
        <label class="sender-field">
          <span>Pgm</span>
          <input type="number" bind:value={program} min="0" max="127" />
        </label>
      {/if}
      <button class="sender-btn" on:click={send} title="Send MIDI message">Send</button>
    </div>
  </div>
</div>

<style>
  .midi-panel {
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

  /* Filter help */
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
  .filter-help code {
    color: var(--accent);
    background: rgba(76, 201, 240, 0.08);
    padding: 0 3px;
    border-radius: 2px;
  }

  /* Column headers */
  .event-section {
    flex: 1;
    display: flex;
    flex-direction: column;
    min-height: 0;
    padding-top: 6px;
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
  }
  .row-tx { background: rgba(76, 201, 240, 0.04); }
  .row-rx { background: rgba(255, 255, 255, 0.015); }

  /* Shared column widths */
  .col-time { min-width: 80px; flex-shrink: 0; }
  .col-ports { min-width: 160px; flex-shrink: 0; overflow: hidden; text-overflow: ellipsis; }
  .col-ch   { min-width: 20px; flex-shrink: 0; }
  .col-type { min-width: 70px; flex-shrink: 0; }
  .col-summary { flex: 1; min-width: 0; overflow: hidden; text-overflow: ellipsis; }
  .col-raw  { flex-shrink: 0; }

  .event-time  { color: var(--muted); }
  .event-ports { color: var(--text); }
  .event-ch    { color: var(--muted); }
  .event-type  { color: var(--accent); }
  .event-summary { color: var(--text); }
  .event-raw   { color: var(--muted); opacity: 0.5; }

  .no-time .col-time { display: none; }

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
  .sender-route-row {
    display: flex;
    gap: 8px;
    align-items: center;
    margin-bottom: 6px;
  }
  .route-arrow {
    color: var(--muted);
    font-size: 0.9rem;
    flex-shrink: 0;
  }
  .sender-msg-row {
    display: flex;
    gap: 6px;
    align-items: center;
    flex-wrap: wrap;
  }
  .sender-select {
    font-family: var(--mono);
    font-size: 0.72rem;
    background: var(--bg-2);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 3px 6px;
    cursor: pointer;
  }
  .sender-select:focus { border-color: var(--accent); outline: none; }
  .sender-field {
    display: flex;
    align-items: center;
    gap: 3px;
    font-size: 0.7rem;
    font-family: var(--mono);
    color: var(--muted);
  }
  .sender-field input {
    width: 42px;
    font-family: var(--mono);
    font-size: 0.72rem;
    background: var(--bg-2);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 3px 4px;
    text-align: center;
  }
  .sender-field select {
    font-family: var(--mono);
    font-size: 0.72rem;
    background: var(--bg-2);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 3px 6px;
    cursor: pointer;
  }
  .sender-field input:focus,
  .sender-field select:focus { border-color: var(--accent); outline: none; }
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
    margin-left: auto;
  }
  .sender-btn:hover { background: rgba(76, 201, 240, 0.12); }

  .custom-port-input {
    width: 72px;
    font-family: var(--mono);
    font-size: 0.72rem;
    background: var(--bg-2);
    color: var(--text);
    border: 1px solid var(--accent);
    border-radius: 4px;
    padding: 3px 4px;
  }
  .custom-port-input:focus { outline: none; border-color: var(--accent); }

  input[type='number'] {
    -moz-appearance: textfield;
    appearance: textfield;
  }
  input[type='number']::-webkit-inner-spin-button,
  input[type='number']::-webkit-outer-spin-button {
    -webkit-appearance: none;
    margin: 0;
  }
</style>
