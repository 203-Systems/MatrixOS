<script>
  import { onMount } from 'svelte'
  import { Close, TrashCan } from 'carbon-icons-svelte'
  import { get } from 'svelte/store'
  import { moduleReady, runtimeStatus } from '../../stores/wasm.js'
  import {
    pythonEvents,
    pythonPanelSessionStartId,
    pythonPanelUploadedScript,
  } from '../../stores/python.js'
  import {
    hasPythonApp,
    isPythonAppActive,
    getPythonSessionMode,
    enterPythonRepl,
    stagePythonScript,
    runStagedPythonScript,
    sendPythonInput,
  } from '../../handles/python.js'

  export let showHero = true
  export let onCloseHero = () => {}

  let fileInput
  let busy = ''
  let toast = null
  let toastTimer = null
  let pythonInstalled = false
  let pythonActive = false
  let pythonMode = 'none'

  function showToast(message, type = 'success') {
    if (toastTimer) clearTimeout(toastTimer)
    toast = { message, type, id: Date.now() }
    toastTimer = setTimeout(() => { toast = null }, 5000)
  }

  function dismissToast() {
    if (toastTimer) clearTimeout(toastTimer)
    toastTimer = null
    toast = null
  }

  function currentPythonMarker() {
    const events = get(pythonEvents)
    return events.length ? events[events.length - 1].id + 1 : 0
  }

  function resetOutputWindow() {
    pythonPanelSessionStartId.set(currentPythonMarker())
  }

  function sanitizeStreamText(text) {
    return String(text || '')
      .replace(/^[A-Za-z]:\\[^\n]*\n?/gm, '')
  }

  function renderTerminalText(text) {
    const lines = [[]]
    let row = 0
    let column = 0
    const stream = sanitizeStreamText(text)

    const ensureLine = () => {
      if (!lines[row]) lines[row] = []
    }

    const writeCharacter = (character) => {
      ensureLine()
      while (lines[row].length < column) lines[row].push(' ')
      lines[row][column] = character
      column += 1
    }

    for (let index = 0; index < stream.length; index += 1) {
      const character = stream[index]

      if (character === '\b') {
        if (column > 0) column -= 1
        continue
      }

      if (character === '\r') {
        column = 0
        continue
      }

      if (character === '\n') {
        row += 1
        column = 0
        ensureLine()
        continue
      }

      if (character === '\t') {
        const spaces = 4 - (column % 4)
        for (let space = 0; space < spaces; space += 1) writeCharacter(' ')
        continue
      }

      if (character === '\x1b' && stream[index + 1] === '[') {
        let sequenceEnd = index + 2
        while (sequenceEnd < stream.length && /[0-9;]/.test(stream[sequenceEnd])) {
          sequenceEnd += 1
        }

        const command = stream[sequenceEnd]
        const values = stream.slice(index + 2, sequenceEnd).split(';').map((value) => Number(value) || 0)
        const amount = values[0] || 1

        if (command === 'D') column = Math.max(0, column - amount)
        else if (command === 'C') column += amount
        else if (command === 'G') column = Math.max(0, amount - 1)
        else if (command === 'K') {
          ensureLine()
          if (values[0] === 2) {
            lines[row] = []
            column = 0
          } else {
            lines[row].length = column
          }
        }

        if (command) index = sequenceEnd
        continue
      }

      if (character < ' ') continue

      writeCharacter(character)
    }

    return lines.map((line) => line.join('').replace(/\s+$/u, '')).join('\n')
  }

  function formatBytes(byteCount) {
    if (byteCount < 1024) return `${byteCount} B`
    if (byteCount < 1024 * 1024) return `${(byteCount / 1024).toFixed(1)} KB`
    return `${(byteCount / (1024 * 1024)).toFixed(1)} MB`
  }

  async function refreshStatus() {
    pythonInstalled = hasPythonApp()
    pythonActive = isPythonAppActive()
    pythonMode = pythonActive ? getPythonSessionMode() : 'none'
  }

  function setNotice(message) {
    showToast(message, 'success')
  }

  function setError(message) {
    showToast(message, 'error')
  }

  async function waitForPythonMode(expectedMode, timeoutMs = 1800) {
    const deadline = Date.now() + timeoutMs
    while (Date.now() < deadline) {
      await refreshStatus()
      if (pythonActive && pythonMode === expectedMode) return true
      await new Promise((resolve) => setTimeout(resolve, 50))
    }

    await refreshStatus()
    return pythonActive && pythonMode === expectedMode
  }

  async function handleEnterRepl() {
    if (!$moduleReady) {
      setError('Runtime is not live yet. Load firmware and wait for the device to come up first.')
      return
    }
    if (!pythonInstalled) {
      setError('This MystrixSim build does not currently expose the Python application.')
      return
    }

    busy = 'repl'
    resetOutputWindow()
    setNotice('Opening Python REPL…')

    try {
      if (!enterPythonRepl()) {
        throw new Error('The runtime rejected the Python REPL launch request.')
      }

      const entered = await waitForPythonMode('repl')
      if (!entered) {
        throw new Error('Python REPL did not become active. Check whether the runtime is still booting or another app is still taking focus.')
      }

      setNotice('Python REPL is ready.')
    } catch (launchError) {
      setError(launchError instanceof Error ? launchError.message : String(launchError))
    } finally {
      busy = ''
    }
  }

  function handleUploadClick() {
    fileInput?.click()
  }

  async function handleFileChange(event) {
    const file = event.currentTarget?.files?.[0]
    event.currentTarget.value = ''
    if (!file) return

    try {
      const text = await file.text()
      pythonPanelUploadedScript.set({
        name: file.name,
        text,
        byteLength: file.size,
        lineCount: Math.max(1, text.split(/\r?\n/).length),
        sizeLabel: formatBytes(file.size),
      })

      const uploadedScript = get(pythonPanelUploadedScript)

      if ($moduleReady) {
        if (!stagePythonScript(uploadedScript.name, uploadedScript.text)) {
          throw new Error('Uploaded file was read, but staging it into the runtime failed.')
        }
        setNotice(`${uploadedScript.name} staged for Python app mode.`)
      } else {
        setNotice(`${uploadedScript.name} loaded locally. Start the runtime before running it.`)
      }
    } catch (uploadError) {
      setError(uploadError instanceof Error ? uploadError.message : String(uploadError))
    }
  }

  async function handleRunUploaded() {
    const uploadedScript = get(pythonPanelUploadedScript)
    if (!uploadedScript) {
      setError('Upload a .py file before trying to run it.')
      return
    }
    if (!$moduleReady) {
      setError('Runtime is not live yet. Load firmware and wait for the device to come up first.')
      return
    }
    if (!pythonInstalled) {
      setError('This MystrixSim build does not currently expose the Python application.')
      return
    }

    busy = 'run'
    resetOutputWindow()
    setNotice(`Launching ${uploadedScript.name}…`)

    try {
      if (!stagePythonScript(uploadedScript.name, uploadedScript.text)) {
        throw new Error('Failed to stage the uploaded Python file into runtime memory.')
      }
      if (!runStagedPythonScript()) {
        throw new Error('The runtime rejected the uploaded Python file launch request.')
      }

      const launched = await waitForPythonMode('app')
      if (!launched) {
        throw new Error(`${uploadedScript.name} did not stay active in app mode. The script may have failed during startup or exited immediately.`)
      }
      setNotice(`${uploadedScript.name} launched. Python output is streaming below.`)
    } catch (runError) {
      setError(runError instanceof Error ? runError.message : String(runError))
    } finally {
      busy = ''
    }
  }

  function clearUploadedScript() {
    pythonPanelUploadedScript.set(null)
    dismissToast()
  }

  function handleTerminalKeydown(event) {
    if (!pythonActive) return
    if (event.ctrlKey) {
      if (event.key === 'c') { event.preventDefault(); sendPythonInput('\x03'); return }
      if (event.key === 'd') { event.preventDefault(); sendPythonInput('\x04'); return }
      return
    }
    if (event.key === 'Enter') { event.preventDefault(); sendPythonInput('\n'); return }
    if (event.key === 'Backspace') { event.preventDefault(); sendPythonInput('\b'); return }
    if (event.key === 'Tab') { event.preventDefault(); sendPythonInput('\t'); return }
    if (event.key.length === 1) { event.preventDefault(); sendPythonInput(event.key) }
  }

  onMount(() => {
    if (get(pythonPanelSessionStartId) === null) {
      pythonPanelSessionStartId.set(currentPythonMarker())
    }
    void refreshStatus()

    const statusTimer = window.setInterval(() => {
      void refreshStatus()
    }, 250)

    return () => {
      window.clearInterval(statusTimer)
    }
  })

  $: sessionPythonEvents = $pythonEvents.filter((event) => event.id >= ($pythonPanelSessionStartId ?? 0))
  $: sessionReplEvents = sessionPythonEvents.filter((event) => event.mode === 'repl' || event.mode === 'unknown')
  $: sessionAppEvents = sessionPythonEvents.filter((event) => event.mode === 'app')
  $: replTranscript = renderTerminalText(sessionReplEvents.map((event) => event.text).join(''))
  $: appLogEvents = sessionAppEvents.map((event) => ({
    ...event,
    cleanText: sanitizeStreamText(event.text),
  }))
  $: pythonStatusLabel = !$moduleReady ? 'Runtime Offline' : pythonInstalled ? 'Available' : 'Missing'
  $: pythonModeLabel = !$moduleReady || !pythonActive
    ? 'Not Running'
    : pythonMode === 'app'
      ? `APP - ${$pythonPanelUploadedScript?.name || 'Unknown'}`
      : pythonMode === 'repl'
        ? 'REPL'
        : 'Unknown'

  let logShell
  let replShell
  let autoScroll = true
  let userScrolledUp = false
  let scrollRafId = null

  function cancelScrollRaf() {
    if (scrollRafId !== null) {
      cancelAnimationFrame(scrollRafId)
      scrollRafId = null
    }
  }

  function scrollToBottom() {
    const shell = pythonMode === 'app' ? logShell : replShell
    if (shell) {
      shell.scrollTop = shell.scrollHeight
      autoScroll = true
      userScrolledUp = false
    }
  }

  function onShellScroll(shell) {
    if (!shell) return
    const near = shell.scrollTop + shell.clientHeight >= shell.scrollHeight - 40
    if (!near) {
      cancelScrollRaf()
      autoScroll = false
      userScrolledUp = true
    } else {
      autoScroll = true
      userScrolledUp = false
    }
  }

  $: if (appLogEvents && autoScroll && logShell) {
    cancelScrollRaf()
    scrollRafId = requestAnimationFrame(() => {
      scrollRafId = null
      if (autoScroll && logShell) logShell.scrollTop = logShell.scrollHeight
    })
  }
  $: if (replTranscript && autoScroll && replShell) {
    cancelScrollRaf()
    scrollRafId = requestAnimationFrame(() => {
      scrollRafId = null
      if (autoScroll && replShell) replShell.scrollTop = replShell.scrollHeight
    })
  }

  // Auto-focus terminal when REPL becomes active so typing works immediately
  $: if (pythonActive && pythonMode === 'repl' && replShell) {
    replShell.focus()
  }
</script>

<div class="tool-surface python-panel">
  {#if showHero}
    <section class="tool-hero">
      <button class="tool-hero-close" on:click={onCloseHero} title="Close">✕</button>
      <div class="tool-hero-title">Python</div>
      <div class="tool-hero-desc">
        Enter the built-in MicroPython runtime in REPL mode, or upload a <code>.py</code> file and launch it in app mode.
        Upload and run stay separate so the script is staged first and only executes after explicit confirmation.
      </div>
    </section>
  {/if}

  <section class="tool-section">
    <div class="tool-section-title">Session</div>
    <div class="tool-grid python-status-grid">
      <div class="tool-card">
        <span class="tool-card-label">Python App</span>
        <span
          class="tool-card-value"
          class:tool-value-live={$moduleReady && pythonInstalled}
          class:tool-value-error={!pythonInstalled && $moduleReady}
          class:tool-value-idle={!$moduleReady}
        >
          {pythonStatusLabel}
        </span>
      </div>
      <div class="tool-card">
        <span class="tool-card-label">Runtime Mode</span>
        <span
          class="tool-card-value"
          class:tool-value-live={pythonActive && pythonMode === 'repl'}
          class:tool-value-idle={!pythonActive || !$moduleReady}
          class:python-mode-app={pythonActive && pythonMode === 'app'}
        >
          {pythonModeLabel}
        </span>
      </div>
    </div>

    {#if !$moduleReady}
      <div class="python-toast python-toast-error">
        <span class="python-toast-msg">Runtime status is currently <strong>{$runtimeStatus}</strong>. Bring the simulator up first.</span>
      </div>
    {/if}

    {#if $moduleReady && !pythonInstalled}
      <div class="python-toast python-toast-error">
        <span class="python-toast-msg">Python is not registered in this MystrixSim build.</span>
      </div>
    {/if}
  </section>

  <section class="tool-section">
    <div class="tool-section-title">Launch</div>
    <div class="python-launch-grid">
      <button
        class="python-launch-btn python-launch-repl"
        disabled={!$moduleReady || !pythonInstalled || busy !== ''}
        on:click={handleEnterRepl}
      >
        {busy === 'repl' ? 'Entering…' : 'Enter REPL'}
      </button>

      {#if $pythonPanelUploadedScript}
        <button
          class="python-launch-btn python-launch-run"
          disabled={!$moduleReady || !pythonInstalled || busy !== ''}
          on:click={handleRunUploaded}
        >
          {busy === 'run' ? 'Running…' : 'Run Script'}
        </button>
      {:else}
        <button
          class="python-launch-btn python-launch-upload"
          disabled={busy !== ''}
          on:click={handleUploadClick}
        >
          Upload .py
        </button>
      {/if}
    </div>

    {#if $pythonPanelUploadedScript}
      <div class="python-file-loaded-card">
        <div class="python-file-loaded-content">
          <span class="python-file-loaded-label">File Loaded</span>
          <span class="python-file-loaded-name">{$pythonPanelUploadedScript.name}</span>
          <span class="python-file-loaded-meta">
            {$pythonPanelUploadedScript.lineCount} lines · {$pythonPanelUploadedScript.sizeLabel}
          </span>
        </div>
        <button
          class="python-file-clear"
          on:click={clearUploadedScript}
          disabled={busy !== ''}
          title="Remove loaded file"
          aria-label="Remove loaded Python file"
        >
          <Close size={20} />
        </button>
      </div>
    {/if}

    <input
      bind:this={fileInput}
      class="python-hidden-input"
      type="file"
      accept=".py,text/x-python,text/plain"
      on:change={handleFileChange}
    />

    {#if toast}
      <div class="python-toast" class:python-toast-error={toast.type === 'error'}>
        <span class="python-toast-msg">{toast.message}</span>
        <button class="python-toast-close" on:click={dismissToast} title="Dismiss">✕</button>
        {#key toast.id}
          <div class="python-toast-bar"></div>
        {/key}
      </div>
    {/if}
  </section>

  <section class="tool-section python-output-section">
    <div class="tool-section-title-row python-output-head">
      <div class="tool-section-title">Terminal</div>
      <button class="python-clear-btn" on:click={resetOutputWindow} title="Clear terminal">
        <TrashCan size={16} />
      </button>
    </div>

    <div class="python-terminal-wrap">
      {#if pythonMode === 'app'}
        <div class="python-log-shell" bind:this={logShell} on:scroll={() => onShellScroll(logShell)}>
          {#if appLogEvents.length === 0}
            <div class="tool-empty python-empty-state">
              No Python app output captured in this window yet.
            </div>
          {:else}
            {#each appLogEvents as event (event.id)}
              <div class="python-log-row">
                <span class="python-log-time">{event.timestamp}</span>
                <span class="python-log-text">{event.cleanText || ' '}</span>
              </div>
            {/each}
          {/if}
        </div>
      {:else}
        <!-- svelte-ignore a11y-noninteractive-element-interactions a11y-no-noninteractive-tabindex -->
        <div
          class="python-repl-terminal"
          bind:this={replShell}
          role="textbox"
          aria-label="Python REPL terminal"
          aria-multiline="true"
          tabindex="0"
          on:scroll={() => onShellScroll(replShell)}
          on:keydown={handleTerminalKeydown}
        >
          <pre class="python-repl-output">{replTranscript || 'No REPL output yet. Enter Python REPL to begin.\n'}</pre>
        </div>
      {/if}
      {#if userScrolledUp}
        <button class="py-scroll-btn" on:click={scrollToBottom} title="Scroll to bottom">↓</button>
      {/if}
    </div>
  </section>
</div>

<style>
  .python-panel {
    --python-terminal-font: Consolas, "Courier New", monospace;
    gap: 14px;
    overflow: hidden;
  }

  .python-mode-app {
    color: #86b8ff;
  }

  .python-status-grid {
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  }

  .python-clear-btn {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    padding: 2px 5px;
    display: inline-flex;
    align-items: center;
  }

  .python-clear-btn:hover {
    color: var(--danger);
    border-color: var(--danger);
  }

  .python-hidden-input {
    display: none;
  }

  .python-launch-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
  }

  .python-launch-btn {
    border-radius: 6px;
    padding: 10px 12px;
    min-height: 52px;
    font: inherit;
    font-size: 0.82rem;
    cursor: pointer;
    transition: opacity 0.12s, background 0.12s, border-color 0.12s;
  }

  .python-launch-btn:disabled {
    opacity: 0.45;
    cursor: not-allowed;
  }

  .python-launch-repl,
  .python-launch-run,
  .python-launch-upload {
    border: 1px solid var(--accent);
    background: rgba(76, 201, 240, 0.12);
    color: var(--accent);
  }

  .python-launch-repl:not(:disabled):hover,
  .python-launch-run:not(:disabled):hover,
  .python-launch-upload:not(:disabled):hover {
    background: rgba(76, 201, 240, 0.2);
  }

  .python-file-loaded-card {
    display: flex;
    align-items: center;
    gap: 12px;
    min-height: 64px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: var(--panel);
    padding: 10px 12px;
  }

  .python-file-loaded-content {
    display: grid;
    gap: 3px;
    min-width: 0;
    flex: 1;
  }

  .python-file-loaded-label {
    color: var(--muted);
    font-size: 0.66rem;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }

  .python-file-loaded-name {
    font-family: var(--mono);
    color: var(--text);
    font-size: 0.82rem;
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .python-file-loaded-meta {
    color: var(--muted);
    font-size: 0.72rem;
  }

  .python-file-clear {
    flex-shrink: 0;
    width: 40px;
    height: 40px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    background: transparent;
    border: none;
    border-radius: 4px;
    color: var(--muted);
    cursor: pointer;
    transition: color 0.12s, background 0.12s;
  }

  .python-file-clear:not(:disabled):hover {
    background: rgba(255, 111, 111, 0.08);
    color: var(--danger);
  }

  .python-file-clear:disabled {
    opacity: 0.45;
    cursor: not-allowed;
  }

  .python-output-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
  }

  .python-output-section {
    flex: 1;
    min-height: 0;
    overflow: hidden;
  }

  .python-terminal-wrap {
    display: flex;
    flex-direction: column;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: #0f1014;
    overflow: hidden;
    position: relative;
    width: 100%;
    flex: 1;
    min-height: 0;
  }

  .python-log-shell,
  .python-repl-terminal {
    flex: 1;
    min-height: 0;
    overflow-y: auto;
    scrollbar-width: thin;
    scrollbar-color: rgba(255,255,255,0.12) transparent;
  }

  .python-repl-terminal {
    cursor: text;
    outline: none;
    font-family: var(--python-terminal-font);
  }

  .python-repl-terminal:focus {
    box-shadow: inset 0 0 0 1px rgba(255,255,255,0.12);
  }

  .python-log-shell {
    display: flex;
    flex-direction: column;
    gap: 1px;
    padding: 4px 0;
  }

  .python-log-row {
    display: flex;
    gap: 8px;
    padding: 3px 6px;
    border-radius: 3px;
    font-family: var(--python-terminal-font);
    font-size: 0.73rem;
    background: rgba(255, 255, 255, 0.015);
    align-items: baseline;
  }

  .python-log-time {
    color: var(--muted);
  }

  .python-log-text {
    color: var(--text);
    white-space: pre-wrap;
    word-break: break-word;
  }

  .python-repl-output {
    margin: 0;
    padding: 10px 12px;
    color: var(--text);
    font-family: var(--python-terminal-font);
    font-size: 0.73rem;
    line-height: 1.55;
    white-space: pre-wrap;
    word-break: break-word;
  }

  .python-hidden-input {
    position: absolute;
    left: 0;
    bottom: 0;
    width: 1px;
    height: 1px;
    opacity: 0;
    pointer-events: none;
    border: none;
    outline: none;
    padding: 0;
    background: transparent;
    color: transparent;
    caret-color: transparent;
  }



  .python-toast {
    position: relative;
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 9px 10px 9px 12px;
    border-radius: 6px;
    border: 1px solid rgba(61, 214, 140, 0.24);
    background: rgba(61, 214, 140, 0.08);
    font-size: 0.76rem;
    line-height: 1.4;
    color: #8ee8b6;
    overflow: hidden;
  }

  .python-toast-error {
    border-color: rgba(255, 111, 111, 0.22);
    background: rgba(255, 111, 111, 0.08);
    color: #ffb7b7;
  }

  .python-toast-msg {
    flex: 1;
    min-width: 0;
  }

  .python-toast-close {
    flex-shrink: 0;
    background: none;
    border: none;
    color: inherit;
    opacity: 0.55;
    font-size: 0.8rem;
    line-height: 1;
    padding: 2px 4px;
    cursor: pointer;
    border-radius: 3px;
  }

  .python-toast-close:hover {
    opacity: 1;
  }

  .python-toast-bar {
    position: absolute;
    bottom: 0;
    left: 0;
    height: 2px;
    background: currentColor;
    opacity: 0.35;
    animation: python-toast-drain 5s linear forwards;
  }

  @keyframes python-toast-drain {
    from { width: 100%; }
    to   { width: 0%; }
  }

  @media (max-width: 720px) {
    .python-log-row {
      grid-template-columns: 1fr;
      gap: 2px;
    }
  }

  .py-scroll-btn {
    position: absolute;
    right: 14px;
    bottom: 14px;
    width: 28px;
    height: 28px;
    border-radius: 50%;
    border: 1px solid var(--border);
    background: var(--panel);
    color: var(--text);
    cursor: pointer;
    font-size: 0.8rem;
    display: flex;
    align-items: center;
    justify-content: center;
  }
  .py-scroll-btn:hover { border-color: var(--accent); }
</style>
