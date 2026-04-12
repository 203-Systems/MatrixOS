<script>
  import { moduleReady } from '../stores/wasm.js'
  import { inputEvents, clearInputEvents } from '../stores/input.js'
  import { Search, TrashCan, Information, Time } from 'carbon-icons-svelte'
  export let showHero = true
  export let onCloseHero = () => {}

  let eventBody
  let autoScroll = true
  let filterQuery = ''
  let showTimestamps = false
  let showFilterHelp = false

  function scrollToBottom() {
    if (eventBody) {
      eventBody.scrollTop = eventBody.scrollHeight
      autoScroll = true
    }
  }

  $: filteredEvents = (() => {
    const q = filterQuery.trim().toLowerCase()
    if (!q) return $inputEvents
    const typeM = q.match(/\btype:(grid|fn|function)\b/)
    const stateM = q.match(/\bstate:(press|release|hold)\b/)
    const plain = q.replace(/\b(type|state):\S+/g, '').trim()
    return $inputEvents.filter(evt => {
      if (typeM) {
        const t = typeM[1]
        if ((t === 'grid') && evt.type !== 'grid') return false
        if ((t === 'fn' || t === 'function') && evt.type !== 'fn') return false
      }
      if (stateM) {
        const s = stateM[1]
        if (s === 'press' && !evt.pressed) return false
        if (s === 'release' && evt.pressed) return false
      }
      if (plain) {
        const typeLabel = evt.type === 'fn' ? 'function key' : 'grid'
        const stateLabel = evt.pressed ? 'press' : 'release'
        const posLabel = evt.type === 'grid' ? `${evt.x},${evt.y}` : ''
        return typeLabel.includes(plain) || stateLabel.includes(plain) || posLabel.includes(plain) || evt.timestamp.includes(plain)
      }
      return true
    })
  })()

  $: if ($inputEvents && autoScroll && eventBody) {
    requestAnimationFrame(() => {
      if (eventBody) eventBody.scrollTop = eventBody.scrollHeight
    })
  }
</script>

<div class="input-panel">
  {#if showHero}
  <div class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">Input</div>
    <div class="tool-hero-desc">Click or tap grid keys to inject input events into the runtime. Monitor the full input event stream in real time.</div>
  </div>
  {/if}
  <div class="filter-bar">
    <div class="filter-search" class:filter-active={filterQuery}>
      <Search size={13} />
      <input
        type="text"
        class="filter-input"
        placeholder="Filter input…"
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
    <button class="clear-btn" on:click={clearInputEvents} title="Clear events">
      <TrashCan size={13} />
    </button>
  </div>

  {#if showFilterHelp}
    <div class="filter-help">
      <span><code>type:grid</code> or <code>type:fn</code> — event type</span>
      <span><code>state:press</code> or <code>state:release</code> — state</span>
      <span>Plain text matches position, type, and state.</span>
    </div>
  {/if}

  <div class="event-section">
    <div class="event-col-header" class:no-time={!showTimestamps}>
      {#if showTimestamps}<span class="col-time">Time</span>{/if}
      <span class="col-type">Source</span>
      <span class="col-event">Event</span>
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
        <div class="empty-msg">{$inputEvents.length === 0 ? 'No input events yet.' : 'No events match the filter.'}</div>
      {:else}
        {#each filteredEvents as evt (evt.id)}
          <div class="event-row" class:event-press={evt.pressed} class:event-release={!evt.pressed} class:no-time={!showTimestamps}>
            {#if showTimestamps}<span class="col-time event-time">{evt.timestamp}</span>{/if}
            <span class="col-type event-type">
              {#if evt.type === 'grid'}
                Grid ({evt.x},{evt.y})
              {:else}
                Function Key
              {/if}
            </span>
            <span class="col-event event-state">
              {evt.pressed ? '▼ Press' : '▲ Release'}
              {#if evt.type === 'grid' && evt.pressed && evt.velocity != null}
                <span class="event-vel"> — Velocity {Math.round((evt.velocity / 127) * 100)}%</span>
              {/if}
            </span>
          </div>
        {/each}
      {/if}
    </div>
  </div>
</div>

<style>
  .input-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    gap: 0;
    padding: 10px 14px;
    overflow: hidden;
  }

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
  .col-time  { min-width: 85px; flex-shrink: 0; }
  .col-type  { min-width: 100px; flex-shrink: 0; }
  .col-event { flex: 1; min-width: 0; }

  .event-body {
    flex: 1;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    gap: 1px;
    font-family: var(--mono);
    font-size: 0.75rem;
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
  .event-press  { background: rgba(76, 201, 240, 0.04); }
  .event-release { background: rgba(255, 255, 255, 0.015); }
  .event-time { color: var(--muted); }
  .event-type { color: var(--accent); }
  .event-state { color: var(--muted); }
  .event-press .event-state { color: #6bdd8b; }
  .event-release .event-state { color: #9ea1ad; }
  .event-vel {
    color: #6bdd8b;
    font-size: 0.7rem;
    opacity: 0.9;
  }
  .empty-msg {
    color: var(--muted);
    font-size: 0.82rem;
    text-align: center;
    padding: 24px;
    font-family: var(--sans);
  }
</style>
