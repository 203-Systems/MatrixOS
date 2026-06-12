<script>
  import { onMount } from 'svelte'
  import { moduleReady, runtimeStatus } from '../../stores/wasm.js'
  import { getApplicationState, launchApplication } from '../../handles/application.js'

  export let showHero = true
  export let onCloseHero = () => {}

  let applicationState = { activeApp: null, activeAppId: null, applications: [], available: false }
  let showHiddenApps = false
  let refreshTimer = 0

  function refreshApplications() {
    applicationState = getApplicationState()
  }

  function appPills(app) {
    const pills = []
    if (app?.system) pills.push('System')
    if (app && app.visible === false) pills.push('Hidden')
    return pills
  }

  function versionLabel(version) {
    if (!version) return ''
    return String(version)
  }

  function versionBadge(version) {
    const label = versionLabel(version)
    return label ? `v${label}` : '-'
  }

  function appColor(app) {
    return app?.color || '#808080'
  }

  function appReference(app) {
    if (app?.reference) return app.reference
    if (app?.author && app?.name) return `${app.author}-${app.name}`
    return '-'
  }

  function appId(app) {
    return app?.idHex || app?.id || '-'
  }

  function launchApp(app) {
    if (!app?.id) return
    launchApplication(app.id)
    window.setTimeout(refreshApplications, 150)
  }

  onMount(() => {
    refreshApplications()
    refreshTimer = window.setInterval(refreshApplications, 500)
    return () => window.clearInterval(refreshTimer)
  })

  $: activeApp = applicationState.activeApp
  $: applicationListAvailable = applicationState.available !== false
  $: appCount = applicationState.applications.length
  $: hiddenCount = applicationState.applications.filter((app) => app.visible === false).length
  $: otherApps = applicationState.applications.filter((app) => {
    if (activeApp && app.id === activeApp.id) return false
    if (!showHiddenApps && app.visible === false) return false
    return true
  })
</script>

<div class="tool-surface app-panel">
  {#if showHero}
    <section class="tool-hero">
      <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
      <div class="tool-hero-title">Application</div>
      <div class="tool-hero-desc">
        Runtime application status, active app metadata, and registered app visibility.
      </div>
    </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Active App</div>
    {#if !$moduleReady}
      <div class="app-state-note">Runtime status is {$runtimeStatus}.</div>
    {:else if !applicationListAvailable}
      <div class="app-state-note">Application metadata is unavailable. Rebuild and reload the runtime package.</div>
    {:else if !activeApp}
      <div class="app-state-note">No active application.</div>
    {:else}
      <div class="app-active-card">
        <div class="app-active-head">
          <span class="app-color-dot" style={`background:${appColor(activeApp)}`}></span>
          <div class="app-active-name-wrap">
            <div class="app-active-title">
              <span class="app-active-name">{activeApp.name}</span>
              {#each appPills(activeApp) as pill}
                <span class="app-pill" class:app-pill-hidden={pill === 'Hidden'}>{pill}</span>
              {/each}
            </div>
            <div class="app-active-author">{activeApp.author || 'Unknown Author'}</div>
          </div>
        </div>

        <div class="tool-grid app-active-grid">
          <div class="tool-card">
            <span class="tool-card-label">APP Reference</span>
            <span class="tool-card-value app-mono">{appReference(activeApp)}</span>
          </div>
          <div class="tool-card">
            <span class="tool-card-label">APP ID</span>
            <span class="tool-card-value app-mono">{appId(activeApp)}</span>
          </div>
          <div class="tool-card">
            <span class="tool-card-label">Version</span>
            <span class="tool-card-value">{versionLabel(activeApp.version) || '-'}</span>
          </div>
        </div>
      </div>
    {/if}
  </section>

  <section class="tool-section app-list-section">
    <div class="tool-section-title-row app-list-title-row">
      <div class="tool-section-title">Other Apps</div>
      <button
        class="app-toggle"
        class:app-toggle-active={showHiddenApps}
        on:click={() => showHiddenApps = !showHiddenApps}
        title="Show hidden apps"
      >
        <span class="app-toggle-box"></span>
        <span>Show Hidden App</span>
      </button>
    </div>

    <div class="app-list-summary">
      <span>{Math.max(0, appCount - (activeApp ? 1 : 0))} apps</span>
      {#if hiddenCount > 0}
        <span>{hiddenCount} hidden</span>
      {/if}
    </div>

    <div class="app-list">
      {#if !$moduleReady}
        <div class="tool-empty app-empty">Runtime is not live.</div>
      {:else if !applicationListAvailable}
        <div class="tool-empty app-empty">Application metadata is unavailable.</div>
      {:else if otherApps.length === 0}
        <div class="tool-empty app-empty">No apps to show.</div>
      {:else}
        {#each otherApps as app (app.id)}
          <div class="app-row" class:app-row-hidden={app.visible === false}>
            <span class="app-row-color" style={`background:${appColor(app)}`}></span>
            <div class="app-row-main">
              <div class="app-row-title">
                <span class="app-row-name">{app.name}</span>
                {#each appPills(app) as pill}
                  <span class="app-pill" class:app-pill-hidden={pill === 'Hidden'}>{pill}</span>
                {/each}
              </div>
              <div class="app-row-sub">
                <span>{appReference(app)}</span>
                <span>{versionBadge(app.version)}</span>
                <span class="app-row-id">{appId(app)}</span>
              </div>
            </div>
            <button class="app-launch-button" on:click={() => launchApp(app)} disabled={activeApp && app.id === activeApp.id}>
              Launch
            </button>
          </div>
        {/each}
      {/if}
    </div>
  </section>
</div>

<style>
  .app-panel {
    gap: 14px;
    min-height: 0;
    overflow: hidden;
  }

  .app-state-note {
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 12px;
    color: var(--muted);
    background: rgba(255, 255, 255, 0.025);
    font-size: 0.86rem;
  }

  .app-active-card {
    border: 1px solid var(--border);
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.025);
    overflow: hidden;
  }

  .app-active-head {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 12px;
    border-bottom: 1px solid var(--border);
  }

  .app-color-dot,
  .app-row-color {
    display: block;
    width: 14px;
    height: 14px;
    border-radius: 50%;
    border: 1px solid rgba(255, 255, 255, 0.24);
    box-shadow: 0 0 10px rgba(255, 255, 255, 0.08);
    flex-shrink: 0;
  }

  .app-active-name-wrap {
    min-width: 0;
  }

  .app-active-title,
  .app-row-title {
    display: flex;
    align-items: center;
    gap: 6px;
    min-width: 0;
  }

  .app-active-name,
  .app-row-name {
    color: var(--text);
    font-weight: 500;
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .app-active-author,
  .app-row-sub {
    color: var(--muted);
    font-size: 0.76rem;
    margin-top: 2px;
    min-width: 0;
  }

  .app-pill {
    border: 1px solid rgba(76, 201, 240, 0.35);
    border-radius: 999px;
    padding: 1px 6px;
    color: #b9ebff;
    background: rgba(76, 201, 240, 0.08);
    font-size: 0.62rem;
    font-weight: 800;
    text-transform: uppercase;
    letter-spacing: 0.04em;
    flex-shrink: 0;
  }

  .app-pill-hidden {
    border-color: rgba(247, 194, 102, 0.35);
    color: #f7c266;
    background: rgba(247, 194, 102, 0.08);
  }

  .app-active-grid {
    grid-template-columns: repeat(3, minmax(0, 1fr));
    padding: 12px;
  }

  .app-mono,
  .app-row-id {
    font-family: var(--mono);
  }

  .app-list-section {
    display: flex;
    flex: 1;
    flex-direction: column;
    min-height: 0;
  }

  .app-list-title-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 10px;
  }

  .app-toggle {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: rgba(255, 255, 255, 0.03);
    color: var(--muted);
    font: inherit;
    font-size: 0.72rem;
    font-weight: 700;
    padding: 4px 8px;
    cursor: pointer;
  }

  .app-toggle:hover,
  .app-toggle-active {
    color: var(--text);
    border-color: rgba(76, 201, 240, 0.45);
    background: rgba(76, 201, 240, 0.08);
  }

  .app-toggle-box {
    width: 9px;
    height: 9px;
    border-radius: 2px;
    border: 1px solid currentColor;
    box-sizing: border-box;
  }

  .app-toggle-active .app-toggle-box {
    background: #4cc9f0;
    border-color: #4cc9f0;
  }

  .app-list-summary {
    display: flex;
    gap: 10px;
    color: var(--muted);
    font-size: 0.72rem;
    margin: 4px 0 8px;
  }

  .app-list {
    display: flex;
    flex: 1;
    flex-direction: column;
    gap: 2px;
    min-height: 0;
    overflow: auto;
    border: 1px solid var(--border);
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.02);
  }

  .app-row,
  .app-empty {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 8px 10px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.06);
  }

  .app-row:last-child {
    border-bottom: 0;
  }

  .app-row-hidden {
    opacity: 0.78;
  }

  .app-row-main {
    flex: 1;
    min-width: 0;
  }

  .app-row-sub {
    display: flex;
    gap: 8px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .app-row-sub span:first-child {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .app-launch-button {
    border: 1px solid rgba(76, 201, 240, 0.36);
    border-radius: 6px;
    background: rgba(76, 201, 240, 0.08);
    color: #b9ebff;
    cursor: pointer;
    flex-shrink: 0;
    font: inherit;
    font-size: 0.72rem;
    font-weight: 700;
    padding: 5px 9px;
  }

  .app-launch-button:hover:not(:disabled) {
    background: rgba(76, 201, 240, 0.14);
    border-color: rgba(76, 201, 240, 0.58);
    color: var(--text);
  }

  .app-launch-button:disabled {
    cursor: default;
    opacity: 0.45;
  }

  .app-empty {
    color: var(--muted);
    border-bottom: 0;
  }

  @media (max-width: 680px) {
    .app-active-grid {
      grid-template-columns: 1fr;
    }

    .app-list-title-row {
      align-items: flex-start;
      flex-direction: column;
    }
  }
</style>
