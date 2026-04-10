<script>
  import { onMount } from 'svelte'
  import { initWasm } from './stores/wasm.js'
  import { hookModuleLogging } from './stores/logs.js'
  import TopBar from './components/TopBar.svelte'
  import LeftNav from './components/LeftNav.svelte'
  import DevicePanel from './components/DevicePanel.svelte'
  import InputPanel from './components/InputPanel.svelte'
  import LogsPanel from './components/LogsPanel.svelte'
  import RuntimePanel from './components/RuntimePanel.svelte'

  let activeSection = 'device'

  const sectionPrefKey = 'matrixos-active-section'

  onMount(() => {
    // Restore last active section
    try {
      const stored = window.localStorage.getItem(sectionPrefKey)
      if (['device', 'input', 'logs', 'runtime'].includes(stored)) {
        activeSection = stored
      }
    } catch {}

    // Hook log capture before WASM init so early messages are captured
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
      {:else if activeSection === 'input'}
        <InputPanel />
      {:else if activeSection === 'logs'}
        <LogsPanel />
      {:else if activeSection === 'runtime'}
        <RuntimePanel />
      {/if}
    </main>
  </div>
</div>
