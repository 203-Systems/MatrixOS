<script>
  let searchQuery = ''
  let activeTag = 'all'

  const tags = ['all', 'Release', 'Beta', 'Nightly', 'InDev']

  const placeholderFirmware = [
    { name: 'Matrix OS 3.3 InDev', version: 'v3.3.0-indev.12', date: '2026-04-10', tag: 'InDev' },
    { name: 'Matrix OS 3.2 Nightly', version: 'v3.2.0-nightly.88', date: '2026-03-28', tag: 'Nightly' },
    { name: 'Matrix OS 3.2 Beta', version: 'v3.2.0-beta.3', date: '2026-03-10', tag: 'Beta' },
    { name: 'Matrix OS 3.1 Release', version: 'v3.1.2', date: '2026-02-14', tag: 'Release' },
    { name: 'Matrix OS 3.0 Release', version: 'v3.0.5', date: '2025-12-01', tag: 'Release' },
    { name: 'Matrix OS 3.0 Beta', version: 'v3.0.0-beta.7', date: '2025-11-15', tag: 'Beta' },
  ]

  $: filteredFirmware = placeholderFirmware.filter(f => {
    const tagMatch = activeTag === 'all' || f.tag === activeTag
    const q = searchQuery.trim().toLowerCase()
    const textMatch = !q || f.name.toLowerCase().includes(q) || f.version.toLowerCase().includes(q)
    return tagMatch && textMatch
  })
</script>

<div class="firmware-page">
  <section class="tool-hero">
    <div class="tool-hero-title">Firmware</div>
    <span class="status-pill status-warn">Under Construction</span>
    <div class="tool-hero-desc">
      Browse and flash MatrixOS firmware releases. Filter by release channel or search by name and version. Flashing requires a connected device.
    </div>
  </section>

  <div class="fw-toolbar">
    <div class="fw-tag-filter">
      {#each tags as tag}
        <button
          class="fw-tag-btn"
          class:fw-tag-active={activeTag === tag}
          on:click={() => activeTag = tag}
        >{tag === 'all' ? 'All' : tag}</button>
      {/each}
    </div>
    <input
      class="fw-search"
      type="search"
      placeholder="Search firmware…"
      bind:value={searchQuery}
    />
  </div>

  <div class="fw-table">
    <div class="fw-header-row">
      <span class="fw-col-name">Name</span>
      <span class="fw-col-ver">Version</span>
      <span class="fw-col-tag">Channel</span>
      <span class="fw-col-date">Build Date</span>
      <span class="fw-col-action"></span>
    </div>
    {#each filteredFirmware as fw}
      <div class="fw-row">
        <span class="fw-col-name fw-name">{fw.name}</span>
        <span class="fw-col-ver fw-mono">{fw.version}</span>
        <span class="fw-col-tag">
          <span class="fw-channel fw-channel-{fw.tag.toLowerCase()}">{fw.tag}</span>
        </span>
        <span class="fw-col-date fw-mono fw-date">{fw.date}</span>
        <span class="fw-col-action">
          <button class="fw-load-btn">Load</button>
        </span>
      </div>
    {/each}
    {#if filteredFirmware.length === 0}
      <div class="fw-empty">No firmware releases match your filter.</div>
    {/if}
  </div>
</div>

<style>
  .firmware-page {
    display: flex;
    flex-direction: column;
    gap: 14px;
    padding: 16px 20px;
    height: 100%;
    overflow-y: auto;
  }
  .fw-toolbar {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
  }
  .fw-tag-filter {
    display: flex;
    gap: 4px;
  }
  .fw-tag-btn {
    background: none;
    border: 1px solid var(--border);
    color: var(--muted);
    font: inherit;
    font-size: 0.75rem;
    font-weight: 500;
    padding: 4px 10px;
    border-radius: 4px;
    cursor: pointer;
    transition: border-color 0.12s, color 0.12s, background 0.12s;
  }
  .fw-tag-btn:hover { color: var(--text); border-color: rgba(255,255,255,0.2); }
  .fw-tag-active {
    border-color: var(--accent);
    color: var(--accent);
    background: rgba(76, 201, 240, 0.07);
  }
  .fw-search {
    flex: 1;
    min-width: 160px;
    max-width: 280px;
    padding: 5px 10px;
    border: 1px solid var(--border);
    border-radius: 4px;
    background: var(--bg-2);
    color: var(--text);
    font: inherit;
    font-size: 0.78rem;
  }
  .fw-search:focus { outline: none; border-color: var(--accent); }
  .fw-table {
    display: flex;
    flex-direction: column;
    gap: 1px;
    border: 1px solid var(--border);
    border-radius: 6px;
    overflow: hidden;
  }
  .fw-header-row, .fw-row {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 7px 12px;
  }
  .fw-header-row {
    font-size: 0.68rem;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.05em;
    color: var(--muted);
    background: var(--panel);
    border-bottom: 1px solid var(--border);
  }
  .fw-row {
    font-size: 0.78rem;
    background: rgba(255,255,255,0.015);
    transition: background 0.1s;
  }
  .fw-row:hover { background: rgba(255,255,255,0.04); }
  .fw-col-name { flex: 1; min-width: 0; }
  .fw-col-ver { width: 160px; flex-shrink: 0; }
  .fw-col-tag { width: 80px; flex-shrink: 0; }
  .fw-col-date { width: 100px; flex-shrink: 0; }
  .fw-col-action { width: 64px; flex-shrink: 0; display: flex; justify-content: flex-end; }
  .fw-name { font-weight: 500; color: var(--text); }
  .fw-mono { font-family: var(--mono); font-size: 0.72rem; color: var(--muted); }
  .fw-date { opacity: 0.7; }
  .fw-channel {
    display: inline-block;
    font-size: 0.66rem;
    font-weight: 600;
    padding: 2px 6px;
    border-radius: 3px;
    text-transform: uppercase;
    letter-spacing: 0.04em;
  }
  .fw-channel-release { background: rgba(61,214,140,0.12); color: #3dd68c; }
  .fw-channel-beta { background: rgba(76,201,240,0.1); color: #4cc9f0; }
  .fw-channel-nightly { background: rgba(247,194,102,0.1); color: #f7c266; }
  .fw-channel-indev { background: rgba(255,107,107,0.1); color: #ff8b8b; }
  .fw-load-btn {
    background: none;
    border: 1px solid var(--border);
    color: var(--muted);
    font: inherit;
    font-size: 0.7rem;
    padding: 3px 10px;
    border-radius: 4px;
    cursor: not-allowed;
    opacity: 0.4;
  }
  .fw-empty {
    padding: 24px;
    text-align: center;
    color: var(--muted);
    font-size: 0.78rem;
    opacity: 0.6;
  }
</style>
