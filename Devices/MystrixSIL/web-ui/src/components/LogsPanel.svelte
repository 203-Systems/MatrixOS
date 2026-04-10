<script>
  import { Search, Close } from 'carbon-icons-svelte'
  import { filteredLogs, logFilter, logLevelFilter, logMessages, clearLogs } from '../stores/logs.js'

  let logBody
  let autoScroll = true
  let userScrolledUp = false
  let showTimestamps = true

  const timestampPrefKey = 'matrixos-log-show-timestamps'

  const levels = [
    { value: 'all',   label: 'All' },
    { value: 'error', label: 'Error' },
    { value: 'warn',  label: 'Warn' },
    { value: 'info',  label: 'Info' },
    { value: 'log',   label: 'Log' },
  ]

  $: if ($filteredLogs && autoScroll && logBody) {
    requestAnimationFrame(() => {
      if (logBody) logBody.scrollTop = logBody.scrollHeight
    })
  }

  try {
    const stored = window.localStorage.getItem(timestampPrefKey)
    if (stored === '0') showTimestamps = false
  } catch {}

  $: try {
    window.localStorage.setItem(timestampPrefKey, showTimestamps ? '1' : '0')
  } catch {}

  function scrollToBottom() {
    if (logBody) {
      logBody.scrollTop = logBody.scrollHeight
      autoScroll = true
      userScrolledUp = false
    }
  }
</script>

<div class="logs-panel">
  <div class="logs-toolbar">
    <div class="logs-search">
      <Search size={14} />
      <input
        type="text"
        class="logs-search-input"
        placeholder="Filter logs…"
        bind:value={$logFilter}
      />
    </div>
    <select class="logs-level-select" bind:value={$logLevelFilter}>
      {#each levels as lvl}
        <option value={lvl.value}>{lvl.label}</option>
      {/each}
    </select>
    <button
      class="logs-toggle"
      class:logs-toggle-active={showTimestamps}
      on:click={() => (showTimestamps = !showTimestamps)}
      title="Toggle timestamps"
    >
      Time
    </button>
    <span class="logs-count">{$filteredLogs.length} / {$logMessages.length}</span>
    <button class="logs-clear" on:click={clearLogs} title="Clear logs">
      <Close size={14} />
    </button>
  </div>

  <div
    class="logs-body"
    bind:this={logBody}
    on:scroll={() => {
      if (!logBody) return
      const near = logBody.scrollTop + logBody.clientHeight >= logBody.scrollHeight - 16
      autoScroll = near
      userScrolledUp = !near
    }}
  >
      {#if $filteredLogs.length === 0}
        <div class="logs-empty">No logs{$logFilter || $logLevelFilter !== 'all' ? ' matching filter' : ' yet'}.</div>
      {:else}
        {#each $filteredLogs as entry (entry.id)}
        <div class="log-row level-{entry.level}" class:log-row-no-time={!showTimestamps}>
          {#if showTimestamps}
          <span class="log-time">{entry.timestamp}</span>
          {/if}
          <span class="log-level-tag">{entry.level}</span>
          <span class="log-text" style={entry.color ? `color:${entry.color}` : ''}>{entry.text}</span>
        </div>
      {/each}
    {/if}
  </div>

  {#if userScrolledUp}
    <button class="scroll-btn" on:click={scrollToBottom} title="Scroll to bottom">↓</button>
  {/if}
</div>

<style>
  .logs-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    padding: 10px 10px 8px;
    gap: 6px;
    overflow: hidden;
    position: relative;
  }
  .logs-toolbar {
    display: flex;
    align-items: center;
    gap: 6px;
    flex-shrink: 0;
  }
  .logs-search {
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
  .logs-search-input {
    flex: 1;
    background: none;
    border: none;
    outline: none;
    color: var(--text);
    font-family: var(--mono);
    font-size: 0.74rem;
    min-width: 0;
  }
  .logs-search-input::placeholder { color: var(--muted); }
  .logs-level-select {
    font-family: inherit;
    font-size: 0.72rem;
    padding: 3px 7px;
    border: 1px solid var(--border);
    border-radius: 5px;
    background: var(--bg-2);
    color: var(--text);
    cursor: pointer;
    outline: none;
  }
  .logs-count {
    font-size: 0.68rem;
    color: var(--muted);
    font-family: var(--mono);
    white-space: nowrap;
  }
  .logs-toggle {
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
  .logs-toggle:hover,
  .logs-toggle-active {
    color: var(--text);
    border-color: var(--accent);
  }
  .logs-clear {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 1px 3px;
    display: inline-flex;
    align-items: center;
  }
  .logs-clear:hover { color: var(--text); border-color: var(--accent); }

  .logs-body {
    flex: 1;
    overflow-y: auto;
    display: flex;
    flex-direction: column;
    gap: 0;
    font-family: var(--mono);
    font-size: 0.72rem;
    scrollbar-width: thin;
    scrollbar-color: rgba(255,255,255,0.12) transparent;
  }
  .log-row {
    display: grid;
    grid-template-columns: 82px 38px minmax(0, 1fr);
    gap: 6px;
    padding: 1px 4px;
    border-radius: 0;
    background: rgba(255, 255, 255, 0.015);
    align-items: start;
    line-height: 1.25;
    border-bottom: 1px solid rgba(255, 255, 255, 0.025);
  }
  .log-row-no-time {
    grid-template-columns: 38px minmax(0, 1fr);
  }
  .log-time {
    color: var(--muted);
    min-width: 0;
    flex-shrink: 0;
    font-size: 0.66rem;
  }
  .log-level-tag {
    min-width: 0;
    font-size: 0.64rem;
    text-transform: uppercase;
    font-weight: 500;
    flex-shrink: 0;
  }
  .level-error .log-level-tag { color: #ff8b8b; }
  .level-warn .log-level-tag { color: #f7c266; }
  .level-info .log-level-tag { color: #86b8ff; }
  .level-log .log-level-tag { color: var(--muted); }
  .log-text {
    word-break: break-word;
    color: var(--text);
    flex: 1;
    min-width: 0;
  }
  .level-error { background: rgba(255, 107, 107, 0.04); }
  .level-warn { background: rgba(247, 194, 102, 0.03); }

  .logs-empty {
    color: var(--muted);
    font-size: 0.82rem;
    text-align: center;
    padding: 40px;
    font-family: var(--sans);
  }

  .scroll-btn {
    position: absolute;
    right: 20px;
    bottom: 20px;
    width: 28px; height: 28px;
    border-radius: 50%;
    border: 1px solid var(--border);
    background: var(--panel);
    color: var(--text);
    cursor: pointer;
    font-size: 0.8rem;
    display: flex;
    align-items: center;
    justify-content: center;
  }
  .scroll-btn:hover { border-color: var(--accent); }
</style>
