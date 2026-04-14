<script>
  import { IS_NODE_BACKED } from '../stores/rpc.js'
  import { buildMetadata } from '../stores/wasm.js'
  import { buildTypeClass } from '../buildMetadata.js'

  let searchQuery = ''
  let activeTag = 'all'
  let localBuildLoaded = true
  let loadedReleaseHash = ''

  const tags = ['all', 'Release', 'Release Candidate', 'Beta', 'Nightly']

  $: localBuildMeta = $buildMetadata

  function handleLocalBuildLoad() {
    localBuildLoaded = true
  }

  function handleReleaseLoad(fw) {
    loadedReleaseHash = fw.buildHash
  }

  const placeholderFirmware = [
    { name: 'Matrix OS 4.0 RC 1', version: 'v4.0.0-rc.1', buildHash: 'a91f3c7', date: '2026-04-12', channel: 'Release Candidate' },
    { name: 'Matrix OS 4.0 Nightly', version: 'v4.0.0-nightly.17', buildHash: 'f54c91a', date: '2026-04-09', channel: 'Nightly' },
    { name: 'Matrix OS 3.9 Beta', version: 'v3.9.0-beta.5', buildHash: '4b108de', date: '2026-03-30', channel: 'Beta' },
    { name: 'Matrix OS 3.8 Release', version: 'v3.8.2', buildHash: '93ac2e4', date: '2026-02-18', channel: 'Release' },
  ]

  $: filteredFirmware = placeholderFirmware.filter((fw) => {
    const tagMatch = activeTag === 'all' || fw.channel === activeTag
    const q = searchQuery.trim().toLowerCase()
    const textMatch = !q || fw.name.toLowerCase().includes(q) || fw.version.toLowerCase().includes(q) || fw.buildHash.toLowerCase().includes(q)
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

  {#if IS_NODE_BACKED}
  <section class="tool-section">
    <div class="tool-section-title">Local Build</div>
    <div class="tool-grid">
      <div class="tool-card">
        <span class="tool-card-label">Name</span>
        <span class="tool-card-value">{localBuildMeta.version}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Build Type</span>
        <span class={`tool-card-value ${buildTypeClass(localBuildMeta.buildType)}`}>{localBuildMeta.buildType}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Build Hash</span>
        <span class="tool-card-value">{localBuildMeta.buildHash}</span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Build Date</span>
        <span class="tool-card-value">{localBuildMeta.buildTime}</span>
      </div>
      <button class="local-build-load-btn" on:click={handleLocalBuildLoad}>
        {localBuildLoaded ? 'Reload' : 'Load'}
      </button>
    </div>
  </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Official Release</div>
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
        placeholder="Search firmware/version/hash..."
        bind:value={searchQuery}
      />
    </div>

    <div class="fw-table">
      <div class="fw-header-row">
        <span class="fw-col-name">Name</span>
        <span class="fw-col-channel">Channel</span>
        <span class="fw-col-ver">Build String</span>
        <span class="fw-col-hash">Build Hash</span>
        <span class="fw-col-date">Release Date</span>
        <span class="fw-col-action"></span>
      </div>
      {#each filteredFirmware as fw}
        <div class="fw-row">
          <span class="fw-col-name fw-name">{fw.name}</span>
          <span class="fw-col-channel"><span class="fw-channel fw-channel-{fw.channel.toLowerCase().replace(/\s+/g, '-')}">{fw.channel}</span></span>
          <span class="fw-col-ver fw-mono">{fw.version}</span>
          <span class="fw-col-hash fw-mono">{fw.buildHash}</span>
          <span class="fw-col-date fw-mono">{fw.date}</span>
          <span class="fw-col-action">
            <button class="fw-row-load-btn" on:click={() => handleReleaseLoad(fw)}>
              {loadedReleaseHash === fw.buildHash ? 'Reload' : 'Load'}
            </button>
          </span>
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
    gap: 24px;
    padding: 24px 28px;
    height: 100%;
    overflow-y: auto;
  }
  .local-build-load-btn {
    border: 1px solid rgba(76, 201, 240, 0.45);
    color: #b9ebff;
    background: rgba(76, 201, 240, 0.12);
    border-radius: 6px;
    font-size: 0.75rem;
    font-weight: 600;
    letter-spacing: 0.03em;
    text-transform: uppercase;
    padding: 10px 12px;
    cursor: pointer;
    transition: background 0.12s, border-color 0.12s;
  }
  .local-build-load-btn:hover {
    background: rgba(76, 201, 240, 0.18);
    border-color: rgba(76, 201, 240, 0.72);
  }
  .build-type-release { color: #5bea8c; }
  .build-type-rc { color: #8db5ff; }
  .build-type-beta { color: #63d8ff; }
  .build-type-nightly { color: #ffd27e; }
  .build-type-indev { color: #ff8f8f; }
  .build-type-neutral { color: #5bea8c; }
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
  .fw-row-load-btn {
    border: 1px solid rgba(76, 201, 240, 0.45);
    color: #b9ebff;
    background: rgba(76, 201, 240, 0.12);
    border-radius: 4px;
    font-size: 0.68rem;
    font-weight: 600;
    letter-spacing: 0.03em;
    text-transform: uppercase;
    padding: 4px 10px;
    cursor: pointer;
    transition: background 0.12s, border-color 0.12s;
  }
  .fw-row-load-btn:hover {
    background: rgba(76, 201, 240, 0.18);
    border-color: rgba(76, 201, 240, 0.72);
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
  .fw-col-channel { width: 150px; flex-shrink: 0; }
  .fw-col-hash { width: 100px; flex-shrink: 0; }
  .fw-col-date { width: 120px; flex-shrink: 0; }
  .fw-col-action { width: 90px; flex-shrink: 0; text-align: right; }
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
  .fw-channel-release { background: rgba(61,214,140,0.12); color: #3dd68c; }
  .fw-channel-release-candidate { background: rgba(140,170,255,0.14); color: #9db9ff; }
  .fw-channel-beta { background: rgba(76,201,240,0.1); color: #4cc9f0; }
  .fw-channel-nightly { background: rgba(247,194,102,0.1); color: #f7c266; }
  .fw-empty {
    padding: 24px;
    text-align: center;
    color: var(--muted);
    font-size: 0.78rem;
    opacity: 0.6;
  }

</style>
