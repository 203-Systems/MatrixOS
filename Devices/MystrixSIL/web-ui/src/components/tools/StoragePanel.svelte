<script>
  import { onMount } from 'svelte'
  import { nvsEntries, nvsConnected, refreshNvs, deleteNvsEntry, clearNvs, downloadNvsExport, importNvsFromFile, filesystemMounted, filesystemPath } from '../../stores/storage.js'
  import { Close, Download, Upload, TrashCan } from 'carbon-icons-svelte'

  let fileInput

  onMount(() => {
    refreshNvs()
  })

  function handleImportClick() {
    fileInput?.click()
  }

  async function handleFileChange(e) {
    const file = e.target.files?.[0]
    if (file) {
      await importNvsFromFile(file)
      e.target.value = ''
    }
  }

  function handleDelete(hash) {
    deleteNvsEntry(hash)
  }
</script>

<div class="storage-panel">
  <!-- NVS Section -->
  <div class="nvs-section">
    <div class="section-header">
      <span class="section-title">NVS Store</span>
      <span class="section-count">{$nvsEntries.length}</span>
      <div class="header-actions">
        <button class="action-btn" on:click={refreshNvs} title="Refresh">↻</button>
        <button class="action-btn" on:click={downloadNvsExport} title="Export">
          <Download size={14} />
        </button>
        <button class="action-btn" on:click={handleImportClick} title="Import">
          <Upload size={14} />
        </button>
        <button class="action-btn action-danger" on:click={clearNvs} title="Clear all">
          <TrashCan size={14} />
        </button>
      </div>
    </div>

    <input
      type="file"
      accept=".bin,.nvs"
      bind:this={fileInput}
      on:change={handleFileChange}
      class="hidden-input"
    />

    <div class="nvs-body">
      {#if !$nvsConnected}
        <div class="empty-msg">NVS bridge not available.</div>
      {:else if $nvsEntries.length === 0}
        <div class="empty-msg">NVS store is empty.</div>
      {:else}
        <div class="nvs-table">
          <div class="nvs-header-row">
            <span class="nvs-col-hash">Hash</span>
            <span class="nvs-col-size">Size</span>
            <span class="nvs-col-value">Value</span>
            <span class="nvs-col-actions"></span>
          </div>
          {#each $nvsEntries as entry (entry.hash)}
            <div class="nvs-row">
              <span class="nvs-col-hash mono">{entry.hashHex}</span>
              <span class="nvs-col-size mono">{entry.size}B</span>
              <span class="nvs-col-value mono" title={entry.rawBytes}>{entry.preview}</span>
              <span class="nvs-col-actions">
                <button class="row-action" on:click={() => handleDelete(entry.hash)} title="Delete entry">
                  <Close size={12} />
                </button>
              </span>
            </div>
          {/each}
        </div>
      {/if}
    </div>
  </div>

  <!-- Filesystem Section -->
  <div class="fs-section">
    <div class="section-header">
      <span class="section-title">Filesystem</span>
      <span class="status-pill" class:status-live={$filesystemMounted} class:status-idle={!$filesystemMounted}>
        {$filesystemMounted ? 'Mounted' : 'Not mounted'}
      </span>
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
    gap: 12px;
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
  .mono { font-family: var(--mono); }
  .nvs-col-hash { width: 110px; flex-shrink: 0; }
  .nvs-col-size { width: 50px; flex-shrink: 0; text-align: right; }
  .nvs-col-value { flex: 1; min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; color: var(--text); }
  .nvs-col-actions { width: 28px; flex-shrink: 0; display: flex; justify-content: center; }
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
  .row-action:hover { color: var(--danger); }
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
  }
  .fs-info {
    display: flex;
    gap: 8px;
    align-items: center;
    font-size: 0.76rem;
  }
  .fs-label { color: var(--muted); font-size: 0.72rem; }
  .fs-path { color: var(--text); }
  .fs-placeholder {
    color: var(--muted);
    font-size: 0.76rem;
    font-style: italic;
    padding: 4px 0;
  }
</style>
