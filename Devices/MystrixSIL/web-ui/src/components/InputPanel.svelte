<script>
  import { get } from 'svelte/store'
  import { moduleReady } from '../stores/wasm.js'
  import { inputEvents, activeGridKeys, fnKeyActive, logInputEvent, clearInputEvents, runtimeGridKeys, runtimeFnActive } from '../stores/input.js'
  import { Close } from 'carbon-icons-svelte'

  let eventBody
  let autoScroll = true

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
  <!-- Active keys snapshot (injection side) -->
  {#if $activeGridKeys.size > 0 || $fnKeyActive}
    <div class="state-section">
      <span class="section-title">Injected</span>
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

  <!-- Runtime-side state (what the OS actually sees) -->
  {#if $runtimeGridKeys.size > 0 || $runtimeFnActive}
    <div class="state-section runtime-state">
      <span class="section-title">Runtime</span>
      <div class="state-chips">
        {#if $runtimeFnActive}
          <span class="state-chip chip-fn chip-runtime">FN</span>
        {/if}
        {#each [...$runtimeGridKeys] as key}
          <span class="state-chip chip-runtime">({key})</span>
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
  .runtime-state .section-title {
    color: #6bdd8b;
  }
  .chip-runtime {
    background: rgba(107, 221, 139, 0.1);
    border: 1px solid rgba(107, 221, 139, 0.2);
    color: #6bdd8b;
  }
  .chip-fn.chip-runtime {
    background: rgba(107, 221, 139, 0.1);
    border-color: rgba(107, 221, 139, 0.2);
    color: #6bdd8b;
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
