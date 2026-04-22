<script>
  import { onMount } from 'svelte'
  import { Search, Download, Upload, TrashCan, Copy, DocumentAdd, ArrowLeft } from 'carbon-icons-svelte'
  import {
    nvsEntries,
    nvsConnected,
    refreshNvs,
    pollNvs,
    deleteNvsEntry,
    clearNvs,
    downloadNvsExport,
    importNvsFromFile,
    filesystemMounted,
    filesystemPath,
    filesystemEntries,
    filesystemBusy,
    filesystemError,
    refreshFilesystem,
    navigateFilesystem,
    goUpFilesystem,
    downloadFilesystemExport,
    importFilesystemArchive,
    uploadFilesystemFile,
    downloadFilesystemFile,
    deleteFilesystemEntry,
    computeNvsHash,
  } from '../../stores/storage.js'

  export let showHero = true
  export let onCloseHero = () => {}

  let nvsImportInput
  let fsArchiveInput
  let fsUploadInput
  let keyInput = ''
  let searchQuery = ''
  let nvsPollTimer
  let expandedHashes = new Set()

  $: keyHash = keyInput.length > 0 ? computeNvsHash(keyInput) : null
  $: keyHashHex = keyHash !== null ? '0x' + keyHash.toString(16).padStart(8, '0').toUpperCase() : ''
  $: if (keyHashHex) searchQuery = keyHashHex
  $: if (!keyInput) searchQuery = ''

  $: filteredEntries = (() => {
    const q = searchQuery.trim().toLowerCase()
    if (!q) return $nvsEntries
    return $nvsEntries.filter((entry) => (
      entry.hashHex.toLowerCase().includes(q) ||
      entry.preview.toLowerCase().includes(q) ||
      String(entry.size).includes(q)
    ))
  })()

  $: fsSegments = (() => {
    const normalized = String($filesystemPath || '/')
    if (normalized === '/') return [{ label: 'root', path: '/' }]

    const parts = normalized.split('/').filter(Boolean)
    let current = ''
    return [
      { label: 'root', path: '/' },
      ...parts.map((segment) => {
        current += `/${segment}`
        return { label: segment, path: current }
      }),
    ]
  })()

  function formatBytes(byteCount, isDir = false) {
    if (isDir) return 'DIR'
    if (byteCount < 1024) return `${byteCount} B`
    if (byteCount < 1024 * 1024) return `${(byteCount / 1024).toFixed(1)} KB`
    return `${(byteCount / (1024 * 1024)).toFixed(1)} MB`
  }

  function handleNvsImportClick() {
    nvsImportInput?.click()
  }

  async function handleNvsFileChange(event) {
    const file = event.currentTarget?.files?.[0]
    event.currentTarget.value = ''
    if (!file) return
    await importNvsFromFile(file)
  }

  function toggleExpand(hash) {
    if (expandedHashes.has(hash)) expandedHashes.delete(hash)
    else expandedHashes.add(hash)
    expandedHashes = expandedHashes
  }

  function copyEntryAsJson(entry) {
    const json = JSON.stringify({ hash: entry.hashHex, value: entry.rawBytes })
    navigator.clipboard?.writeText(json)
  }

  function handleFsArchiveClick() {
    fsArchiveInput?.click()
  }

  function handleFsUploadClick() {
    fsUploadInput?.click()
  }

  async function handleFsArchiveChange(event) {
    const file = event.currentTarget?.files?.[0]
    event.currentTarget.value = ''
    if (!file) return
    await importFilesystemArchive(file, $filesystemPath)
  }

  async function handleFsUploadChange(event) {
    const file = event.currentTarget?.files?.[0]
    event.currentTarget.value = ''
    if (!file) return
    await uploadFilesystemFile(file, $filesystemPath)
  }

  function openFilesystemEntry(entry) {
    if (entry.isDir) {
      navigateFilesystem(entry.path)
    }
  }

  onMount(() => {
    refreshNvs()
    refreshFilesystem('/')
    nvsPollTimer = setInterval(pollNvs, 1500)
    return () => clearInterval(nvsPollTimer)
  })
</script>

<div class="tool-surface">
  {#if showHero}
    <section class="tool-hero">
      <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
      <div class="tool-hero-title">Storage</div>
      <div class="tool-hero-desc">
        Inspect NVS entries and browse the MystrixSim virtual filesystem. Export or import zip snapshots,
        upload single files into the current directory, and download or delete files directly from the browser.
      </div>
    </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title-row">
      <div class="tool-section-title">Non-Volatile Storage (NVS)</div>
      <div class="header-actions">
          <button class="action-btn" on:click={refreshNvs} title="Refresh">↻</button>
          <button class="action-btn" on:click={downloadNvsExport} title="Export NVS">
            <Download size={16} />
          </button>
          <button class="action-btn" on:click={handleNvsImportClick} title="Import NVS">
            <Upload size={16} />
          </button>
          <button class="action-btn action-danger" on:click={clearNvs} title="Clear all NVS entries">
            <TrashCan size={16} />
          </button>
        </div>
      </div>

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
          <Search size={16} />
          <input
            class="search-input"
            type="text"
            bind:value={searchQuery}
            placeholder="Filter by hash or value…"
            spellcheck="false"
          />
        </div>
      </div>

      <input
        type="file"
        accept=".bin,.nvs"
        bind:this={nvsImportInput}
        on:change={handleNvsFileChange}
        class="hidden-input"
      />

      <div class="section-body">
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
                    <Copy size={16} />
                  </button>
                  <button class="row-action" on:click={() => deleteNvsEntry(entry.hash)} title="Delete entry">
                    <TrashCan size={16} />
                  </button>
                </span>
              </div>
            {/each}
          </div>
        {/if}
      </div>

      {#if keyHash !== null}
        <div class="section-note">Keys are stored as FNV-1a hashes. Original strings are not recoverable from the NVS backend.</div>
      {/if}
  </section>

  <section class="tool-section">
    <div class="tool-section-title-row">
      <div class="tool-section-title">File System</div>
      <div class="header-actions">
          <button class="action-btn" on:click={() => refreshFilesystem($filesystemPath)} title="Refresh directory">↻</button>
          <button class="action-btn" on:click={() => downloadFilesystemExport($filesystemPath)} title="Export current directory">
            <Download size={16} />
          </button>
          <button class="action-btn" on:click={handleFsArchiveClick} title="Import zip into current directory">
            <Upload size={16} />
          </button>
          <button class="action-btn" on:click={handleFsUploadClick} title="Upload file into current directory">
            <DocumentAdd size={16} />
          </button>
        </div>
      </div>

      <div class="fs-toolbar">
        <button class="path-btn" on:click={goUpFilesystem} disabled={$filesystemPath === '/'} title="Go up"><ArrowLeft size={16} /></button>
        <div class="fs-breadcrumbs">
          {#each fsSegments as segment, index (segment.path)}
            <button class="crumb-btn" on:click={() => navigateFilesystem(segment.path)}>
              {segment.label}
            </button>
            {#if index < fsSegments.length - 1}
              <span class="crumb-sep">/</span>
            {/if}
          {/each}
        </div>
      </div>

      <input
        type="file"
        accept=".zip"
        bind:this={fsArchiveInput}
        on:change={handleFsArchiveChange}
        class="hidden-input"
      />
      <input
        type="file"
        bind:this={fsUploadInput}
        on:change={handleFsUploadChange}
        class="hidden-input"
      />

      {#if $filesystemBusy}
        <div class="fs-state-row">Busy: {$filesystemBusy}</div>
      {/if}
      {#if $filesystemError}
        <div class="fs-error-row">{$filesystemError}</div>
      {/if}

      <div class="section-body">
        {#if !$filesystemMounted}
          <div class="empty-msg">Filesystem is not mounted yet.</div>
        {:else if $filesystemEntries.length === 0}
          <div class="empty-msg">This directory is empty.</div>
        {:else}
          <div class="fs-table">
            <div class="fs-header-row">
              <span class="fs-col-name">Name</span>
              <span class="fs-col-size">Size</span>
              <span class="fs-col-actions"></span>
            </div>
            {#each $filesystemEntries as entry (entry.path)}
              <div class="fs-row">
                <button
                  class="fs-col-name fs-entry-btn"
                  class:fs-entry-dir={entry.isDir}
                  on:click={() => openFilesystemEntry(entry)}
                  title={entry.path}
                >
                  <span class="fs-entry-kind">{entry.isDir ? 'DIR' : 'FILE'}</span>
                  <span class="fs-entry-label mono">{entry.name}</span>
                </button>
                <span class="fs-col-size mono">{formatBytes(entry.size, entry.isDir)}</span>
                <span class="fs-col-actions">
                  {#if !entry.isDir}
                    <button class="row-action" on:click={() => downloadFilesystemFile(entry)} title="Download file">
                      <Download size={16} />
                    </button>
                  {/if}
                  <button class="row-action" on:click={() => deleteFilesystemEntry(entry)} title="Delete entry">
                    <TrashCan size={16} />
                  </button>
                </span>
              </div>
            {/each}
          </div>
        {/if}
      </div>

      <div class="section-note">
        Export and import operate on the current directory as a zip archive. File upload writes a single file into the current directory.
      </div>
  </section>
</div>

<style>
  .header-actions {
    margin-left: auto;
    display: flex;
    gap: 4px;
  }

  .action-btn,
  .path-btn,
  .crumb-btn {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 6px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 4px;
    font-size: 0.74rem;
    font-family: inherit;
    min-height: 24px;
  }

  .action-btn:hover,
  .path-btn:hover,
  .crumb-btn:hover {
    color: var(--text);
    border-color: var(--accent);
  }

  .action-btn:disabled,
  .path-btn:disabled {
    opacity: 0.45;
    cursor: not-allowed;
  }

  .action-danger:hover {
    color: var(--danger);
    border-color: var(--danger);
  }

  .hidden-input {
    display: none;
  }

  .section-body {
    max-height: 260px;
    overflow: auto;
    scrollbar-width: thin;
    scrollbar-color: rgba(255, 255, 255, 0.12) transparent;
  }

  .nvs-toolbar,
  .fs-toolbar {
    display: flex;
    align-items: center;
    gap: 6px;
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

  .key-input::placeholder,
  .search-input::placeholder {
    color: var(--muted);
    opacity: 0.55;
  }

  .key-input:focus,
  .search-box:focus-within {
    border-color: var(--accent);
  }

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

  .mono {
    font-family: var(--mono);
  }

  .nvs-table,
  .fs-table {
    display: flex;
    flex-direction: column;
    gap: 1px;
  }

  .nvs-header-row,
  .nvs-row,
  .fs-header-row,
  .fs-row {
    display: flex;
    gap: 8px;
    align-items: center;
    padding: 5px 6px;
    border-radius: 4px;
  }

  .nvs-header-row,
  .fs-header-row {
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

  .nvs-row,
  .fs-row {
    font-size: 0.73rem;
    background: rgba(255, 255, 255, 0.015);
  }

  .nvs-row:hover,
  .fs-row:hover {
    background: rgba(255, 255, 255, 0.04);
  }

  .nvs-row-highlight {
    background: rgba(76, 201, 240, 0.06) !important;
  }

  .nvs-col-hash { width: 92px; flex-shrink: 0; }
  .nvs-col-size { width: 52px; flex-shrink: 0; text-align: right; }
  .nvs-col-value { flex: 1; min-width: 0; display: flex; align-items: center; gap: 4px; }
  .nvs-col-actions,
  .fs-col-actions { width: 56px; flex-shrink: 0; display: flex; justify-content: flex-end; gap: 2px; }

  .nvs-hex-val { min-width: 0; color: var(--text); }
  .nvs-hex-truncated { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .nvs-hex-val:not(.nvs-hex-truncated) { white-space: pre-wrap; word-break: break-all; }

  .nvs-expand-btn,
  .row-action {
    background: none;
    border: none;
    color: var(--muted);
    cursor: pointer;
    padding: 1px 3px;
    display: inline-flex;
    align-items: center;
    border-radius: 3px;
  }

  .nvs-expand-btn {
    font-size: 0.55rem;
    opacity: 0.7;
  }

  .row-action {
    opacity: 0.65;
  }

  .nvs-row:hover .row-action,
  .fs-row:hover .row-action {
    opacity: 1;
  }

  .row-action:hover,
  .nvs-expand-btn:hover {
    color: var(--accent);
  }

  .fs-breadcrumbs {
    display: flex;
    align-items: center;
    gap: 4px;
    min-width: 0;
    flex: 1;
    overflow-x: auto;
    padding: 1px 0;
  }

  .crumb-sep {
    color: var(--muted);
    opacity: 0.45;
    font-family: var(--mono);
  }

  .fs-col-name {
    flex: 1;
    min-width: 0;
  }

  .fs-col-size {
    width: 64px;
    flex-shrink: 0;
    text-align: right;
  }

  .fs-entry-btn {
    border: none;
    background: none;
    padding: 0;
    color: inherit;
    cursor: pointer;
    display: flex;
    align-items: center;
    gap: 8px;
    min-width: 0;
    text-align: left;
  }

  .fs-entry-btn:hover .fs-entry-label {
    color: var(--accent);
  }

  .fs-entry-kind {
    flex-shrink: 0;
    width: 34px;
    color: var(--muted);
    font-size: 0.62rem;
    letter-spacing: 0.04em;
    text-transform: uppercase;
  }

  .fs-entry-dir .fs-entry-kind {
    color: #f7c266;
  }

  .fs-entry-label {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .section-note,
  .fs-state-row,
  .fs-error-row {
    margin-top: 6px;
    font-size: 0.68rem;
    flex-shrink: 0;
  }

  .section-note {
    color: var(--muted);
    opacity: 0.65;
    font-style: italic;
  }

  .fs-state-row {
    color: var(--muted);
  }

  .fs-error-row {
    color: #ff8b8b;
  }

  .empty-msg {
    color: var(--muted);
    font-size: 0.82rem;
    text-align: center;
    padding: 24px;
  }
</style>
