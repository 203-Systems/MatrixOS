<script>
  import { onMount, onDestroy } from 'svelte'
  import { nvsEntries, nvsConnected, refreshNvs, pollNvs, deleteNvsEntry, clearNvs, downloadNvsExport, importNvsFromFile, filesystemMounted, filesystemPath, computeNvsHash } from '../../stores/storage.js'
  import { Search, Download, Upload, TrashCan, Copy } from 'carbon-icons-svelte'
  export let showHero = true
  export let onCloseHero = () => {}

  let fileInput
  let keyInput = ''
  let searchQuery = ''
  let pollTimer
  // Track which entries are expanded (showing full hex value)
  let expandedHashes = new Set()

  // Derive both numeric and hex forms so template can use either
  $: keyHash = keyInput.length > 0 ? computeNvsHash(keyInput) : null
  $: keyHashHex = keyHash !== null ? '0x' + keyHash.toString(16).padStart(8, '0').toUpperCase() : ''

  // Automatically pipe computed hash into search box
  $: if (keyHashHex) searchQuery = keyHashHex
  $: if (!keyInput) searchQuery = ''

  // Filter entries by search query (matches hash hex, size, or value preview)
  $: filteredEntries = (() => {
    const q = searchQuery.trim().toLowerCase()
    if (!q) return $nvsEntries
    return $nvsEntries.filter(e =>
      e.hashHex.toLowerCase().includes(q) ||
      e.preview.toLowerCase().includes(q) ||
      String(e.size).includes(q)
    )
  })()

  onMount(() => {
    refreshNvs()
    // Poll for runtime-side NVS writes every 1.5 s
    pollTimer = setInterval(pollNvs, 1500)
    return () => clearInterval(pollTimer)
  })

  function handleImportClick() { fileInput?.click() }

  async function handleFileChange(e) {
    const file = e.target.files?.[0]
    if (file) {
      await importNvsFromFile(file)
      e.target.value = ''
    }
  }

  function toggleExpand(hash) {
    if (expandedHashes.has(hash)) {
      expandedHashes.delete(hash)
    } else {
      expandedHashes.add(hash)
    }
    expandedHashes = expandedHashes
  }

  function copyEntryAsJson(entry) {
    const json = JSON.stringify({ hash: entry.hashHex, value: entry.rawBytes })
    navigator.clipboard?.writeText(json)
  }
</script>

<div class="storage-panel">
  {#if showHero}
  <div class="tool-hero">
    <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
    <div class="tool-hero-title">Storage</div>
    <div class="tool-hero-desc">Inspect NVS key-value entries and the virtual filesystem. Look up keys by name, view raw hex values, and export/import snapshots.</div>
  </div>
  {/if}
  <!-- NVS Section -->
  <div class="nvs-section">
    <!-- Header row -->
    <div class="section-header">
      <span class="section-title">NVS</span>
      <span class="section-subtitle">hash → value</span>
      <span class="section-count">{filteredEntries.length}{filteredEntries.length !== $nvsEntries.length ? ` / ${$nvsEntries.length}` : ''}</span>
      <div class="header-actions">
        <button class="action-btn" on:click={refreshNvs} title="Refresh">↻</button>
        <button class="action-btn" on:click={downloadNvsExport} title="Export">
          <Download size={14} />
        </button>
        <button class="action-btn" on:click={handleImportClick} title="Import">
          <Upload size={14} />
        </button>
        <button class="action-btn action-danger" on:click={clearNvs} title="Clear all NVS entries">
          <TrashCan size={14} />
        </button>
      </div>
    </div>

    <!-- Integrated key→hash + search toolbar -->
    <div class="nvs-toolbar">
      <div class="key-lookup">
        <input
          class="key-input"
          type="text"
          bind:value={keyInput}
          placeholder="Key → hash…"
          spellcheck="false"
          title="Type a key string — hash is computed live and applied to search"
        />
      </div>
      <div class="search-box">
        <Search size={12} />
        <input
          class="search-input"
          type="text"
          bind:value={searchQuery}
          placeholder="Filter by hash or value…"
          spellcheck="false"
        />
      </div>
    </div>

    <input type="file" accept=".bin,.nvs" bind:this={fileInput} on:change={handleFileChange} class="hidden-input" />

    <div class="nvs-body">
      {#if !$nvsConnected}
        <div class="empty-msg">NVS bridge not available.</div>
      {:else if $nvsEntries.length === 0}
        <div class="empty-msg">NVS store is empty.</div>
      {:else if filteredEntries.length === 0}
        <div class="empty-msg">No entries match the filter.</div>
      {:else}
        <div class="nvs-table">
          <div class="nvs-header-row">
            <span class="nvs-col-hash">Hash</span>
            <span class="nvs-col-size">Size</span>
            <span class="nvs-col-value">Value (hex)</span>
            <span class="nvs-col-actions"></span>
          </div>
          {#each filteredEntries as entry (entry.hash)}
            <div class="nvs-row" class:nvs-row-highlight={keyHash !== null && entry.hash === keyHash}>
              <span class="nvs-col-hash mono">{entry.hashHex}</span>
              <span class="nvs-col-size mono">{entry.size}B</span>
              <span class="nvs-col-value mono">
                <span
                  class="nvs-hex-val"
                  class:nvs-hex-truncated={!expandedHashes.has(entry.hash)}
                  title={entry.rawBytes}
                >{entry.rawBytes || '—'}</span>
                {#if entry.rawBytes && entry.rawBytes.length > 24}
                  <button class="nvs-expand-btn" on:click={() => toggleExpand(entry.hash)}>
                    {expandedHashes.has(entry.hash) ? '▲' : '▼'}
                  </button>
                {/if}
              </span>
              <span class="nvs-col-actions">
                <button class="row-action" on:click={() => copyEntryAsJson(entry)} title="Copy as JSON">
                  <Copy size={11} />
                </button>
                <button class="row-action" on:click={() => deleteNvsEntry(entry.hash)} title="Delete entry">
                  <TrashCan size={11} />
                </button>
              </span>
            </div>
          {/each}
        </div>
      {/if}
    </div>
    {#if keyHash !== null}
      <div class="nvs-note">Keys are stored as FNV-1a hashes. Original strings are not recoverable from the NVS backend.</div>
    {/if}
  </div>

  <!-- Filesystem Section -->
  <div class="fs-section">
    <div class="section-header">
      <span class="section-title">Filesystem</span>
      <span class="fs-status">{$filesystemMounted ? 'Mounted' : 'Not mounted'}</span>
    </div>
    {#if $filesystemMounted}
      <div class="fs-info">
        <span class="fs-label">Path:</span>
        <span class="fs-path mono">{$filesystemPath}</span>
      </div>
    {:else}
      <div class="fs-placeholder">Virtual filesystem not available in SIL mode.</div>
    {/if}
  </div>
</div>

<style>
  .storage-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    gap: 0;
    padding: 14px;
    overflow: hidden;
  }

  /* Header */
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
  .section-subtitle {
    font-size: 0.68rem;
    font-family: var(--mono);
    color: var(--muted);
    opacity: 0.7;
  }
  .section-count {
    font-size: 0.72rem;
    color: var(--muted);
    font-family: var(--mono);
  }
  .header-actions {
    margin-left: auto;
    display: flex;
    gap: 4px;
  }
  .action-btn {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 5px;
    display: inline-flex;
    align-items: center;
    font-size: 0.78rem;
  }
  .action-btn:hover { color: var(--text); border-color: var(--accent); }
  .action-danger:hover { color: var(--danger); border-color: var(--danger); }
  .hidden-input { display: none; }

  /* Integrated key→hash + search toolbar */
  .nvs-toolbar {
    display: flex;
    gap: 6px;
    align-items: center;
    margin-bottom: 6px;
    flex-shrink: 0;
    flex-wrap: wrap;
  }
  .key-lookup {
    display: flex;
    align-items: center;
    gap: 5px;
    flex: 1;
    min-width: 180px;
  }
  .key-input {
    flex: 1;
    font-family: var(--mono);
    font-size: 0.72rem;
    background: var(--bg-2);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 4px;
    padding: 3px 7px;
    outline: none;
  }
  .key-input::placeholder { color: var(--muted); opacity: 0.5; }
  .key-input:focus { border-color: var(--accent); }
  .search-box {
    display: flex;
    align-items: center;
    gap: 5px;
    flex: 1;
    min-width: 120px;
    padding: 3px 7px;
    border: 1px solid var(--border);
    border-radius: 4px;
    background: var(--bg-2);
    color: var(--muted);
  }
  .search-box:focus-within { border-color: var(--accent); }
  .search-input {
    flex: 1;
    background: none;
    border: none;
    outline: none;
    color: var(--text);
    font-family: var(--mono);
    font-size: 0.72rem;
    min-width: 0;
  }
  .search-input::placeholder { color: var(--muted); }

  /* NVS table */
  .nvs-section {
    flex: 1;
    display: flex;
    flex-direction: column;
    min-height: 0;
  }
  .nvs-body {
    flex: 1;
    overflow-y: auto;
    scrollbar-width: thin;
    scrollbar-color: rgba(255,255,255,0.12) transparent;
  }
  .nvs-table {
    display: flex;
    flex-direction: column;
    gap: 1px;
  }
  .nvs-header-row, .nvs-row {
    display: flex;
    gap: 8px;
    padding: 4px 6px;
    align-items: center;
    border-radius: 3px;
  }
  .nvs-header-row {
    font-size: 0.68rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: var(--muted);
    border-bottom: 1px solid var(--border);
    position: sticky;
    top: 0;
    background: var(--panel);
    z-index: 1;
  }
  .nvs-row {
    font-size: 0.73rem;
    background: rgba(255, 255, 255, 0.015);
  }
  .nvs-row:hover { background: rgba(255, 255, 255, 0.04); }
  .nvs-row-highlight {
    background: rgba(76, 201, 240, 0.06) !important;
    border-radius: 3px;
  }
  .mono { font-family: var(--mono); }
  .nvs-col-hash { width: 72px; flex-shrink: 0; }
  .nvs-col-size { width: 50px; flex-shrink: 0; text-align: right; }
  .nvs-col-value { flex: 1; min-width: 0; display: flex; align-items: center; gap: 4px; }
  .nvs-hex-val { min-width: 0; color: var(--text); }
  .nvs-hex-truncated { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .nvs-hex-val:not(.nvs-hex-truncated) { white-space: pre-wrap; word-break: break-all; }
  .nvs-expand-btn {
    flex-shrink: 0;
    background: none;
    border: none;
    color: var(--muted);
    cursor: pointer;
    font-size: 0.55rem;
    padding: 0 2px;
    opacity: 0.6;
  }
  .nvs-expand-btn:hover { opacity: 1; }
  .nvs-col-actions { width: 52px; flex-shrink: 0; display: flex; justify-content: flex-end; gap: 2px; }
  .row-action {
    background: none;
    border: none;
    color: var(--muted);
    cursor: pointer;
    padding: 1px 3px;
    display: inline-flex;
    align-items: center;
    border-radius: 3px;
    opacity: 0.5;
  }
  .nvs-row:hover .row-action { opacity: 1; }
  .row-action:hover { color: var(--accent); }
  .nvs-note {
    margin-top: 4px;
    font-size: 0.66rem;
    color: var(--muted);
    opacity: 0.55;
    font-style: italic;
    flex-shrink: 0;
  }
  .empty-msg {
    color: var(--muted);
    font-size: 0.82rem;
    text-align: center;
    padding: 24px;
    font-family: var(--sans);
  }

  /* Filesystem */
  .fs-section {
    flex-shrink: 0;
    border-top: 1px solid var(--border);
    padding-top: 10px;
    margin-top: 10px;
  }
  .fs-info {
    display: flex;
    gap: 8px;
    align-items: center;
    font-size: 0.76rem;
  }
  .fs-label { color: var(--muted); font-size: 0.72rem; }
  .fs-path { color: var(--text); font-family: var(--mono); }
  .fs-placeholder {
    color: var(--muted);
    font-size: 0.76rem;
    font-style: italic;
    padding: 4px 0;
  }
</style>