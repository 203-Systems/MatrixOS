<script>
  import { Search, TrashCan, Time } from 'carbon-icons-svelte'
  import { filteredLogs, logFilter, logLevelFilter, logMessages, clearLogs } from '../stores/logs.js'
  export let showHero = true
  export let onCloseHero = () => {}

  let logBody
  let autoScroll = true
  let userScrolledUp = false
  let showTimestamps = false

  const timestampPrefKey = 'matrixos-log-show-timestamps'

  const levels = [
    { value: 'all',   label: 'All' },
    { value: 'error', label: 'Error' },
    { value: 'warn',  label: 'Warn' },
    { value: 'info',  label: 'Info' },
    { value: 'log',   label: 'Log' },
  ]

  const levelTagMap = { D: 'Debug', I: 'Info', W: 'Warn', E: 'Error' }
  const levelTagColor = { D: '#5ad4ff', I: '#6bdd8b', W: '#f7c266', E: '#ff8b8b' }

  function parseLogLine(text) {
    const m = text.match(/^([DIWE])\s+\((\d+)\)\s+([^:]+):\s+(.+)/)
    if (m) {
      return {
        parsed: true,
        levelTag: levelTagMap[m[1]] || m[1],
        levelColor: levelTagColor[m[1]] || null,
        source: m[3].trim(),
        message: m[4].trim()
      }
    }
    return { parsed: false, levelTag: 'Unknown', levelColor: null, source: 'Unknown', message: text }
  }

  $: if ($filteredLogs && autoScroll && logBody) {
    requestAnimationFrame(() => {
      if (logBody) logBody.scrollTop = logBody.scrollHeight
    })
  }

  try {
    const stored = window.localStorage.getItem(timestampPrefKey)
    if (stored === '1') showTimestamps = true
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
  {#if showHero}
  <div class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">Logs</div>
    <div class="tool-hero-desc">Live MatrixOS runtime log stream. Filter by level or keyword. Timestamps can be toggled on and off.</div>
  </div>
  {/if}
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
      <Time size={13} />
    </button>
    <button class="logs-clear" on:click={clearLogs} title="Clear logs">
      <TrashCan size={13} />
    </button>
  </div>

  <div class="event-col-header" class:no-time={!showTimestamps}>
    {#if showTimestamps}<span class="col-time">Time</span>{/if}
    <span class="col-level">Level</span>
    <span class="col-source">Tag</span>
    <span class="col-msg">Message</span>
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
          {@const parsed = parseLogLine(entry.text)}
          <div class="log-row level-{entry.level}" class:no-time={!showTimestamps}>
            {#if showTimestamps}
            <span class="col-time log-time">{entry.timestamp}</span>
            {/if}
            <span class="col-level log-level-tag" style={parsed.levelColor ? `color:${parsed.levelColor}` : ''}>{parsed.levelTag}</span>
            <span class="col-source log-source">{parsed.source}</span>
            <span class="col-msg log-text">{parsed.message}</span>
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
  .logs-toggle {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 3px 6px;
    display: inline-flex;
    align-items: center;
  }
  .logs-toggle:hover,
  .logs-toggle-active {
    color: var(--text);
    border-color: var(--accent);
  }
  .logs-toggle-active { color: var(--accent); }
  .logs-clear {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 5px;
    display: inline-flex;
    align-items: center;
  }
  .logs-clear:hover { color: var(--danger); border-color: var(--danger); }

  .event-col-header {
    display: flex;
    gap: 6px;
    padding: 2px 4px 4px;
    font-size: 0.64rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--muted);
    opacity: 0.65;
    border-bottom: 1px solid var(--border);
    flex-shrink: 0;
  }

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
    display: flex;
    gap: 6px;
    padding: 1px 4px;
    border-radius: 0;
    background: rgba(255, 255, 255, 0.015);
    align-items: start;
    line-height: 1.25;
    border-bottom: 1px solid rgba(255, 255, 255, 0.025);
  }
  .col-time   { min-width: 82px; flex-shrink: 0; font-size: 0.66rem; }
  .col-level  { min-width: 44px; flex-shrink: 0; font-size: 0.64rem; font-weight: 600; text-transform: uppercase; }
  .col-source { width: 72px; flex-shrink: 0; font-size: 0.68rem; word-break: break-word; white-space: normal; }
  .col-msg    { flex: 1; min-width: 0; }

  .log-time   { color: var(--muted); }
  .log-source { color: var(--text); opacity: 0.7; }
  .log-text   { word-break: break-word; color: var(--text); }

  .level-error { background: rgba(255, 107, 107, 0.04); }
  .level-warn  { background: rgba(247, 194, 102, 0.03); }

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
