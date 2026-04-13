<script>
  let searchQuery = ''
  let activeTag = 'all'

  const tags = ['all', 'Stable', 'Beta', 'Nightly', 'InDev']

  const placeholderFirmware = [
    { name: 'Matrix OS 4.0 InDev', version: 'v4.0.0-indev.4', date: '2026-04-12', tag: 'InDev' },
    { name: 'Matrix OS 4.0 Nightly', version: 'v4.0.0-nightly.17', date: '2026-04-09', tag: 'Nightly' },
    { name: 'Matrix OS 3.9 Beta', version: 'v3.9.0-beta.5', date: '2026-03-30', tag: 'Beta' },
    { name: 'Matrix OS 3.8 Stable', version: 'v3.8.2', date: '2026-02-18', tag: 'Stable' },
  ]

  $: filteredFirmware = placeholderFirmware.filter((fw) => {
    const tagMatch = activeTag === 'all' || fw.tag === activeTag
    const q = searchQuery.trim().toLowerCase()
    const textMatch = !q || fw.name.toLowerCase().includes(q) || fw.version.toLowerCase().includes(q)
    return tagMatch && textMatch
  })
</script>

<div class="firmware-page">
  <section class="tool-hero">
    <div class="tool-hero-title-row">
      <div class="tool-hero-title">Firmware</div>
      <span class="status-pill status-warn">Under Construction</span>
    </div>
    <div class="tool-hero-desc">
      Under construction. Browse placeholder Matrix OS releases by channel, search by name or version, and prepare for future flash workflows.
    </div>
  </section>

  <section class="tool-section">
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
        placeholder="Search firmware..."
        bind:value={searchQuery}
      />
    </div>

    <div class="fw-table">
      <div class="fw-header-row">
        <span class="fw-col-name">Name</span>
        <span class="fw-col-ver">Release Version</span>
        <span class="fw-col-tag">Tag</span>
        <span class="fw-col-date">Release Date</span>
      </div>
      {#each filteredFirmware as fw}
        <div class="fw-row">
          <span class="fw-col-name fw-name">{fw.name}</span>
          <span class="fw-col-ver fw-mono">{fw.version}</span>
          <span class="fw-col-tag"><span class="fw-channel fw-channel-{fw.tag.toLowerCase()}">{fw.tag}</span></span>
          <span class="fw-col-date fw-mono">{fw.date}</span>
        </div>
      {/each}
      {#if filteredFirmware.length === 0}
        <div class="fw-empty">No firmware releases match your filter.</div>
      {/if}
    </div>
  </section>
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
  .fw-col-ver { width: 180px; flex-shrink: 0; }
  .fw-col-tag { width: 84px; flex-shrink: 0; }
  .fw-col-date { width: 120px; flex-shrink: 0; }
  .fw-name { font-weight: 500; color: var(--text); }
  .fw-mono { font-family: var(--mono); font-size: 0.72rem; color: var(--muted); }
  .fw-channel {
    display: inline-block;
    font-size: 0.66rem;
    font-weight: 600;
    padding: 2px 6px;
    border-radius: 3px;
    text-transform: uppercase;
    letter-spacing: 0.04em;
  }
  .fw-channel-stable { background: rgba(61,214,140,0.12); color: #3dd68c; }
  .fw-channel-beta { background: rgba(76,201,240,0.1); color: #4cc9f0; }
  .fw-channel-nightly { background: rgba(247,194,102,0.1); color: #f7c266; }
  .fw-channel-indev { background: rgba(255,107,107,0.1); color: #ff8b8b; }
  .fw-empty {
    padding: 24px;
    text-align: center;
    color: var(--muted);
    font-size: 0.78rem;
    opacity: 0.6;
  }
</style>
