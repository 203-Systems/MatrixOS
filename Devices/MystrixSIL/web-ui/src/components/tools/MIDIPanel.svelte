<script>
  import { midiEvents, midiConnected, midiPorts, clearMidiEvents, sendMidiNote, sendMidiCC, sendMidiProgramChange } from '../../stores/midi.js'
  import { browserCapabilities } from '../../stores/tooling.js'
  import { Close } from 'carbon-icons-svelte'

  let eventBody
  let autoScroll = true

  // Sender state
  let msgType = 'noteOn'
  let channel = 1
  let note = 60
  let velocity = 100
  let controller = 0
  let ccValue = 64
  let program = 0
  let targetPortId = 0x0100

  $: midiLabel = $browserCapabilities.detected
    ? ($browserCapabilities.midi ? 'Available' : 'Unavailable')
    : 'Detecting'

  $: midiTone = !$browserCapabilities.detected ? 'idle'
    : $browserCapabilities.midi ? 'live' : 'warn'

  $: bridgeTone = $midiConnected ? 'live' : 'idle'
  $: bridgeLabel = $midiConnected ? 'Connected' : 'Not connected'

  // Auto-scroll on new events
  $: if ($midiEvents && autoScroll && eventBody) {
    requestAnimationFrame(() => {
      if (eventBody) eventBody.scrollTop = eventBody.scrollHeight
    })
  }

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
    const port = targetPortId
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
  <!-- Status bar -->
  <div class="status-bar">
    <span class="status-item">
      Bridge: <span class="tool-value-{bridgeTone}">{bridgeLabel}</span>
    </span>
    <span class="status-sep">·</span>
    <span class="status-item">
      Web MIDI: <span class="tool-value-{midiTone}">{midiLabel}</span>
    </span>
  </div>

  <!-- Event log -->
  <div class="event-section">
    <div class="section-header">
      <span class="section-title">MIDI Traffic</span>
      <span class="section-count">{$midiEvents.length}</span>
      <button class="section-action" on:click={clearMidiEvents} title="Clear events">
        <Close size={14} />
      </button>
    </div>
    <div
      class="event-body"
      bind:this={eventBody}
      on:scroll={handleScroll}
    >
      {#if $midiEvents.length === 0}
        <div class="empty-msg">No MIDI traffic captured yet.</div>
      {:else}
        {#each $midiEvents as evt (evt.id)}
          <div class="event-row" class:row-tx={evt.direction === 'TX'} class:row-rx={evt.direction === 'RX'}>
            <span class="event-time">{evt.timestamp}</span>
            <span class="event-dir" class:dir-tx={evt.direction === 'TX'} class:dir-rx={evt.direction === 'RX'}>{evt.direction}</span>
            <span class="event-port">{evt.srcPort}</span>
            <span class="event-arrow">→</span>
            <span class="event-port">{evt.dstPort}</span>
            <span class="event-type">{evt.msgType}{evt.channel != null ? ` ch${evt.channel}` : ''}</span>
            <span class="event-summary">{evt.summary}</span>
            <span class="event-raw">{evt.rawBytes}</span>
          </div>
        {/each}
      {/if}
    </div>
  </div>

  <!-- Sender -->
  <div class="sender-section">
    <div class="sender-row">
      <select bind:value={msgType} class="sender-select">
        <option value="noteOn">Note On</option>
        <option value="noteOff">Note Off</option>
        <option value="cc">CC</option>
        <option value="program">Program</option>
      </select>

      <label class="sender-field">
        <span>Ch</span>
        <input type="number" bind:value={channel} min="1" max="16" />
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

      <select bind:value={targetPortId} class="sender-select">
        {#each $midiPorts as port}
          <option value={port.id}>{port.name}</option>
        {/each}
      </select>

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
  }
  .status-sep { opacity: 0.3; }

  /* Event log header + body (mirrors InputPanel) */
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
  }
  .row-tx { background: rgba(76, 201, 240, 0.04); }
  .row-rx { background: rgba(255, 255, 255, 0.015); }

  .event-time { color: var(--muted); min-width: 80px; flex-shrink: 0; }
  .event-dir { min-width: 20px; font-weight: 600; flex-shrink: 0; }
  .dir-tx { color: var(--accent); }
  .dir-rx { color: #6bdd8b; }
  .event-port { color: var(--text); min-width: 40px; flex-shrink: 0; }
  .event-arrow { color: var(--muted); opacity: 0.4; flex-shrink: 0; }
  .event-type { color: var(--accent); min-width: 70px; flex-shrink: 0; }
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
    flex-wrap: wrap;
  }
  .sender-select {
    font-family: var(--mono);
    font-size: 0.72rem;
    background: var(--bg);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 3px 6px;
    cursor: pointer;
  }
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
    background: var(--bg);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 3px 4px;
    text-align: center;
  }
  .sender-field input:focus { border-color: var(--accent); outline: none; }
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
    margin-left: auto;
  }
  .sender-btn:hover { opacity: 0.85; }
</style>
