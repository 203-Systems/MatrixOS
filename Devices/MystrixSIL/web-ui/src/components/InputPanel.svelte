<script>
  import { get } from 'svelte/store'
  import { moduleReady, sendGridKey, sendFnKey } from '../stores/wasm.js'
  import { inputEvents, activeGridKeys, fnKeyActive, logInputEvent, clearInputEvents } from '../stores/input.js'
  import { Close } from 'carbon-icons-svelte'

  const gridSize = 8
  const gridSlots = Array.from({ length: gridSize })

  let eventBody
  let autoScroll = true

  let injecting = {}
  let fnInjecting = false

  function handleGridInject(x, y, pressed) {
    if (!get(moduleReady)) return
    sendGridKey(x, y, pressed)
    logInputEvent('grid', x, y, pressed)
    if (pressed) {
      injecting = { ...injecting, [`${x},${y}`]: true }
    } else {
      const next = { ...injecting }
      delete next[`${x},${y}`]
      injecting = next
    }
  }

  function handleFnInject(pressed) {
    if (!get(moduleReady)) return
    sendFnKey(pressed)
    logInputEvent('fn', 0, 0, pressed)
    fnInjecting = pressed
  }

  function scrollToBottom() {
    if (eventBody) {
      eventBody.scrollTop = eventBody.scrollHeight
      autoScroll = true
    }
  }

  $: if ($inputEvents && autoScroll && eventBody) {
    requestAnimationFrame(() => {
      if (eventBody) eventBody.scrollTop = eventBody.scrollHeight
    })
  }
</script>

<div class="input-panel">
  <!-- Injection controls -->
  <div class="inject-section">
    <div class="section-header">
      <span class="section-title">Inject</span>
      <span class="section-hint">click grid cells or FN</span>
    </div>
    <div class="inject-controls">
      <div class="inject-grid">
        {#each gridSlots as _, y}
          <div class="inject-row">
            {#each gridSlots as _, x}
              <button
                class="inject-cell"
                class:inject-active={$activeGridKeys.has(`${x},${y}`)}
                on:pointerdown|preventDefault={() => handleGridInject(x, y, true)}
                on:pointerup|preventDefault={() => handleGridInject(x, y, false)}
                on:pointerleave={() => {
                  if (injecting[`${x},${y}`]) handleGridInject(x, y, false)
                }}
                title="({x},{y})"
              ></button>
            {/each}
          </div>
        {/each}
      </div>
      <button
        class="inject-fn"
        class:inject-fn-active={$fnKeyActive}
        on:pointerdown|preventDefault={() => handleFnInject(true)}
        on:pointerup|preventDefault={() => handleFnInject(false)}
        on:pointerleave={() => { if (fnInjecting) handleFnInject(false) }}
      >FN</button>
    </div>
  </div>

  <!-- Active keys snapshot -->
  {#if $activeGridKeys.size > 0 || $fnKeyActive}
    <div class="state-section">
      <span class="section-title">Active</span>
      <div class="state-chips">
        {#if $fnKeyActive}
          <span class="state-chip chip-fn">FN</span>
        {/if}
        {#each [...$activeGridKeys] as key}
          <span class="state-chip">({key})</span>
        {/each}
      </div>
    </div>
  {/if}

  <!-- Event log -->
  <div class="event-section">
    <div class="section-header">
      <span class="section-title">Events</span>
      <span class="section-count">{$inputEvents.length}</span>
      <button class="section-action" on:click={clearInputEvents} title="Clear events">
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
      {#if $inputEvents.length === 0}
        <div class="empty-msg">No input events yet.</div>
      {:else}
        {#each $inputEvents as evt (evt.id)}
          <div class="event-row" class:event-press={evt.pressed} class:event-release={!evt.pressed}>
            <span class="event-time">{evt.timestamp}</span>
            <span class="event-type">{evt.type === 'fn' ? 'FN' : `Grid`}</span>
            {#if evt.type === 'grid'}
              <span class="event-pos">({evt.x},{evt.y})</span>
            {/if}
            <span class="event-state">{evt.pressed ? '▼ press' : '▲ release'}</span>
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
    gap: 12px;
    padding: 14px;
    overflow: hidden;
  }
  .section-header {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 8px;
  }
  .section-title {
    font-weight: 600;
    font-size: 0.82rem;
    letter-spacing: 0.03em;
    text-transform: uppercase;
    color: var(--text);
  }
  .section-hint {
    font-size: 0.72rem;
    color: var(--muted);
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

  /* Injection grid */
  .inject-section { flex-shrink: 0; }
  .inject-controls {
    display: flex;
    align-items: flex-start;
    gap: 10px;
  }
  .inject-grid {
    display: flex;
    flex-direction: column;
    gap: 2px;
    flex-shrink: 0;
  }
  .inject-row {
    display: flex;
    gap: 2px;
  }
  .inject-cell {
    width: 28px; height: 28px;
    border: 1px solid var(--border);
    border-radius: 3px;
    background: var(--bg-2);
    cursor: pointer;
    padding: 0;
    transition: background 0.1s, border-color 0.1s;
  }
  .inject-cell:hover {
    border-color: var(--accent);
    background: rgba(76, 201, 240, 0.1);
  }
  .inject-active {
    background: rgba(76, 201, 240, 0.25) !important;
    border-color: var(--accent) !important;
  }
  .inject-fn {
    width: 40px; height: 40px;
    border: 1px solid var(--border);
    border-radius: 4px;
    background: var(--bg-2);
    color: var(--muted);
    cursor: pointer;
    font-size: 0.72rem;
    font-family: inherit;
    font-weight: 600;
    align-self: center;
  }
  .inject-fn:hover { border-color: var(--accent-2); }
  .inject-fn-active {
    background: rgba(156, 107, 255, 0.2) !important;
    border-color: var(--accent-2) !important;
    color: var(--text) !important;
  }

  /* State snapshot */
  .state-section {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-shrink: 0;
    padding: 6px 0;
    border-top: 1px solid var(--border);
    border-bottom: 1px solid var(--border);
    flex-wrap: wrap;
  }
  .state-chips { display: flex; gap: 4px; flex-wrap: wrap; }
  .state-chip {
    font-size: 0.7rem;
    font-family: var(--mono);
    padding: 1px 6px;
    border-radius: 3px;
    background: rgba(76, 201, 240, 0.1);
    border: 1px solid rgba(76, 201, 240, 0.2);
    color: var(--accent);
  }
  .chip-fn {
    background: rgba(156, 107, 255, 0.1);
    border-color: rgba(156, 107, 255, 0.2);
    color: var(--accent-2);
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
  .event-press { background: rgba(76, 201, 240, 0.04); }
  .event-release { background: rgba(255, 255, 255, 0.015); }
  .event-time { color: var(--muted); min-width: 85px; }
  .event-type { color: var(--accent); min-width: 36px; }
  .event-pos { color: var(--text); min-width: 42px; }
  .event-state { color: var(--muted); }
  .event-press .event-state { color: #6bdd8b; }
  .event-release .event-state { color: #9ea1ad; }
  .empty-msg {
    color: var(--muted);
    font-size: 0.82rem;
    text-align: center;
    padding: 24px;
    font-family: var(--sans);
  }
</style>
