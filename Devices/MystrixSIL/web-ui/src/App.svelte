<script>
  import { onMount } from 'svelte'
  import { initWasm } from './stores/wasm.js'
  import { hookModuleLogging } from './stores/logs.js'
  import TopBar from './components/TopBar.svelte'
  import LeftNav from './components/LeftNav.svelte'
  import DevicePanel from './components/DevicePanel.svelte'
  import SettingsPage from './components/SettingsPage.svelte'
  import FirmwarePage from './components/FirmwarePage.svelte'
  import ToolTray from './components/ToolTray.svelte'
  import ToolPanelStack from './components/ToolPanelStack.svelte'

  let activeSection = 'device'

  const sectionPrefKey = 'matrixos-active-section'

  onMount(() => {
    try {
      const stored = window.localStorage.getItem(sectionPrefKey)
      if (['device', 'settings', 'firmware'].includes(stored)) {
        activeSection = stored
      }
    } catch {}

    const restoreLogging = hookModuleLogging()
    const restoreWasm = initWasm()

    return () => {
      if (restoreWasm) restoreWasm()
      if (restoreLogging) restoreLogging()
    }
  })

  $: {
    try { window.localStorage.setItem(sectionPrefKey, activeSection) } catch {}
  }
</script>

<svelte:head>
  <title>MystrixSIL — MatrixOS Emulator</title>
</svelte:head>

<div class="dashboard">
  <TopBar />

  <div class="dashboard-body">
    <LeftNav bind:active={activeSection} />

    <main class="workspace">
      {#if activeSection === 'device'}
        <DevicePanel />
      {:else if activeSection === 'settings'}
        <SettingsPage />
      {:else if activeSection === 'firmware'}
        <FirmwarePage />
      {/if}
    </main>

    {#if activeSection === 'device'}
      <ToolPanelStack />
      <ToolTray />
    {/if}
  </div>
</div>
