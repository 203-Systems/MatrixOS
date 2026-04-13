<script>
  import { onMount } from 'svelte'
  import { initWasm } from './stores/wasm.js'
  import { hookModuleLogging } from './stores/logs.js'
  import { initRpc } from './stores/rpc.js'
  import { initWsBridge } from './stores/wsbridge.js'
  import TopBar from './components/TopBar.svelte'
  import LeftNav from './components/LeftNav.svelte'
  import DevicePanel from './components/DevicePanel.svelte'
  import ConnectionPage from './components/ConnectionPage.svelte'
  import FirmwarePage from './components/FirmwarePage.svelte'
  import ToolTray from './components/ToolTray.svelte'
  import ToolPanelStack from './components/ToolPanelStack.svelte'
  import { detectBrowserCapabilities } from './stores/tooling.js'

  let activeSection = 'device'

  const sectionPrefKey = 'matrixos-active-section'

  onMount(() => {
    try {
      const stored = window.localStorage.getItem(sectionPrefKey)
      if (['device', 'connection', 'firmware'].includes(stored)) {
        activeSection = stored
      }
    } catch {}

    detectBrowserCapabilities()

    const restoreLogging = hookModuleLogging()
    const restoreWasm = initWasm()
    initRpc()
    initWsBridge()

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
  <title>Matrix OS Developer Toolkit</title>
</svelte:head>

<div class="dashboard">
  <TopBar />

  <div class="dashboard-body">
    <LeftNav bind:active={activeSection} />

    <main class="workspace">
      {#if activeSection === 'device'}
        <DevicePanel />
      {:else if activeSection === 'connection'}
        <ConnectionPage />
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
