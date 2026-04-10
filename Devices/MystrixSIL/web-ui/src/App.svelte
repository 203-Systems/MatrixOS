<script>
  import { onMount } from 'svelte'
  import {
    Settings,
    ArrowsHorizontal,
    Restart,
    Terminal,
    LogoDiscord,
    Help,
    Music
  } from 'carbon-icons-svelte'

  const fallbackSize = 8
  const offColor = 'rgb(160, 160, 160)'
  const underglowOffColor = 'rgba(0, 0, 0, 0)'
  const edgeSlots = Array.from({ length: 8 })
  const baseOverlay = 72
  const glowSoftAlpha = 0.35
  const glowHardAlpha = 0.75
  const highlightBoost = 70
  const highlightAlpha = 0.85
  const highlightMidAlpha = 0.45
  const ansiRegex = /\x1B\[[0-?]*[ -/]*[@-~]/g
  const ansiColorMap = {
    30: '#60656f',
    31: '#ff6b6b',
    32: '#6bdd8b',
    33: '#f7c266',
    34: '#6ba7ff',
    35: '#d47fff',
    36: '#5ad4ff',
    37: '#e6e6ea',
    90: '#9ea1ad',
    91: '#ff8b8b',
    92: '#7ff7a1',
    93: '#ffd27f',
    94: '#86b8ff',
    95: '#e2a0ff',
    96: '#8de5ff',
    97: '#ffffff'
  }

  let grid
  let moduleReady = false
  let status = 'Waiting for MatrixOS runtime...'
  let moduleRef = null
  let rafId = 0
  let activePointerId = null
  let fnPointerId = null
  let activeCell = null
  let wasmSignature = null
  let reloadTimer = 0
  let keypadEls = Array(64).fill(null)
  let lastColors = Array(64).fill('')
  let underglowEls = Array(32).fill(null)
  let lastUnderglowColors = Array(32).fill('')
  let wasmMissing = false
  let wasmMissingLogged = false
  let activePanel = null
  let consoleMessages = []
  let restoreConsole = null
  let logCounter = 0
  const consolePrefKey = 'matrixos-demo-active-panel'
  let consoleBody
  let autoScroll = true
  let userScrolledUp = false
  let versionLabel = '...'

  const clamp = (value) => Math.max(0, Math.min(255, value))

  const overlayChannel = (value) =>
    Math.round(baseOverlay + (255 - baseOverlay) * (clamp(value) / 255))

  const applyLedColor = (r, g, b, w) => {
    let rr = clamp(r + w)
    let gg = clamp(g + w)
    let bb = clamp(b + w)
    const isOff = rr === 0 && gg === 0 && bb === 0

    if (!isOff) {
      rr = overlayChannel(rr)
      gg = overlayChannel(gg)
      bb = overlayChannel(bb)
    }

    return { r: rr, g: gg, b: bb, isOff }
  }

  const applyUnderglowColor = (r, g, b, w) => {
    const rr = clamp(r + w)
    const gg = clamp(g + w)
    const bb = clamp(b + w)
    const isOff = rr === 0 && gg === 0 && bb === 0
    return { r: rr, g: gg, b: bb, isOff }
  }

  const getCornerClip = (x, y) => {
    switch (x + y * 10) {
      case 43:
        return 'polygon(80% 0, 100% 20%, 100% 100%, 0 100%, 0 0)'
      case 44:
        return 'polygon(20% 0, 100% 0, 100% 100%, 0 100%, 0 20%)'
      case 33:
        return 'polygon(100% 0, 100% 80%, 80% 100%, 0 100%, 0 0)'
      case 34:
        return 'polygon(100% 0, 100% 100%, 20% 100%, 0 80%, 0 0)'
      default:
        return ''
    }
  }

  const getUnderglowIndex = (x, y) => {
    if (x === 8 && y >= 0 && y < 8) {
      return 7 - y
    }
    if (y === -1 && x >= 0 && x < 8) {
      return 8 + (7 - x)
    }
    if (x === -1 && y >= 0 && y < 8) {
      return 16 + y
    }
    if (y === 8 && x >= 0 && x < 8) {
      return 24 + x
    }
    return 0
  }

  const setWasmMissing = (message) => {
    wasmMissing = true
    status = message
    if (!wasmMissingLogged) {
      console.error(message)
      wasmMissingLogged = true
    }
  }

  const isHtmlResponse = (response) => {
    const contentType = response.headers.get('content-type') || ''
    return contentType.includes('text/html')
  }

  const checkWasmAvailability = async () => {
    try {
      const response = await fetch('/MatrixOSHost.wasm', {
        method: 'HEAD',
        cache: 'no-store'
      })
      if (!response.ok && response.status === 404) {
        setWasmMissing('Matrix OS Image Missing')
        return false
      }
      if (response.ok && isHtmlResponse(response)) {
        setWasmMissing('Matrix OS Image Missing')
        return false
      }
      if (response.ok) {
        wasmMissing = false
      }
      return response.ok
    } catch (error) {
      return false
    }
  }

  const hookModuleAbort = () => {
    const previousAbort = moduleRef?.onAbort
    if (moduleRef) {
      moduleRef.onAbort = (what) => {
        if (typeof previousAbort === 'function') {
          previousAbort(what)
        }
        setWasmMissing('Matrix OS Image Missing')
      }
    }
    return () => {
      if (moduleRef) {
        moduleRef.onAbort = previousAbort
      }
    }
  }

  const normalizeLog = (raw) => {
    const matches = [...raw.matchAll(/\x1B\[([0-9;]+)m/g)]
    let color = null
    if (matches.length > 0) {
      const last = matches[matches.length - 1][1]
      const parts = last.split(';').map((v) => parseInt(v, 10)).filter((v) => !Number.isNaN(v))
      const code = parts.find((v) => v !== 0)
      if (code && ansiColorMap[code]) {
        color = ansiColorMap[code]
      }
    }
    const text = raw.replace(ansiRegex, '').trim()
    return { text, color }
  }

  const isMatrixOSLog = (text) => {
    if (!text) {
      return false
    }
    if (text.startsWith('[MystrixSIL]')) {
      return true
    }
    return /^[DIWEV]\s*\(\d+\)/.test(text)
  }

  const pushConsoleMessage = (level, args, source = 'console') => {
    const toCleanString = (arg) => {
      if (arg === null || arg === undefined) {
        return String(arg)
      }
      if (typeof arg === 'object') {
        try {
          return JSON.stringify(arg)
        } catch (error) {
          return '[object]'
        }
      }
      return String(arg)
    }

    const raw = args.map(toCleanString).join(' ')
    const { text, color } = normalizeLog(raw)

    if (source !== 'matrixos' && !isMatrixOSLog(text)) {
      return
    }

    // Suppress noisy worker/pthread failures we don't want to show in-panel
    const suppressed =
      text.includes('worker sent an error!') ||
      text.includes('RuntimeError: table index is out of bounds')
    if (suppressed || !text) {
      return
    }

    const levelColor = {
      error: ansiColorMap[91],
      warn: ansiColorMap[93],
      info: ansiColorMap[96],
      log: null
    }[level]

    let prefixColor = null
    const prefixMatch = text.match(/^([DIWE])\s*[(:]/)
    if (prefixMatch) {
      const tag = prefixMatch[1]
      if (tag === 'E') prefixColor = ansiColorMap[91]
      else if (tag === 'W') prefixColor = ansiColorMap[93]
      else if (tag === 'I') prefixColor = ansiColorMap[92]
      else if (tag === 'D') prefixColor = ansiColorMap[96]
    }

    const entry = { id: logCounter++, level, text, color: color || prefixColor || levelColor }
    const trimmed = consoleMessages.slice(-199)
    consoleMessages = [...trimmed, entry]

    if (autoScroll && consoleBody) {
      requestAnimationFrame(() => {
        if (consoleBody) {
          consoleBody.scrollTop = consoleBody.scrollHeight
        }
      })
    }
  }

  const hookModuleLogging = () => {
    window.MatrixOSLogDispatch = (level, args) => {
      pushConsoleMessage(level, args, 'matrixos')
    }
    if (window.MatrixOSLogBuffer && Array.isArray(window.MatrixOSLogBuffer)) {
      window.MatrixOSLogBuffer.forEach((entry) => {
        if (entry && entry.level && entry.args) {
          pushConsoleMessage(entry.level, entry.args, 'matrixos')
        }
      })
      window.MatrixOSLogBuffer.length = 0
    }
    return () => {
      window.MatrixOSLogDispatch = null
    }
  }

  const renderFrame = () => {
    rafId = requestAnimationFrame(renderFrame)
    if (!moduleReady || !moduleRef || !moduleRef.calledRun) {
      return
    }

    if (
      moduleRef._MatrixOS_Wasm_KeypadTick &&
      (activePointerId !== null || fnPointerId !== null)
    ) {
      moduleRef._MatrixOS_Wasm_KeypadTick()
    }

    const heap = moduleRef.HEAPU8 || window.HEAPU8
    if (!heap || !moduleRef._MatrixOS_Wasm_GetFrameBuffer) {
      return
    }

    const width = moduleRef._MatrixOS_Wasm_GetWidth?.() ?? fallbackSize
    const height = moduleRef._MatrixOS_Wasm_GetHeight?.() ?? fallbackSize
    const byteLength = moduleRef._MatrixOS_Wasm_GetFrameBufferByteLength?.() ?? 0
    const ptr = moduleRef._MatrixOS_Wasm_GetFrameBuffer?.() ?? 0

    if (!ptr || !byteLength) {
      return
    }

    const data = heap.subarray(ptr, ptr + byteLength)
    const totalCount = Math.floor(byteLength / 4)
    const gridCount = Math.min(width * height, keypadEls.length, totalCount)

    for (let i = 0; i < gridCount; i += 1) {
      const base = i * 4
      const { r, g, b, isOff } = applyLedColor(
        data[base],
        data[base + 1],
        data[base + 2],
        data[base + 3]
      )
      const color = isOff ? offColor : `rgb(${r}, ${g}, ${b})`
      if (color === lastColors[i]) {
        continue
      }
      lastColors[i] = color
      const el = keypadEls[i]
      if (!el) {
        continue
      }
      if (isOff) {
        el.style.setProperty('--key-color', offColor)
        el.style.removeProperty('--key-glow')
        el.style.removeProperty('--key-glow-filter')
      } else {
        const hr = clamp(r + highlightBoost)
        const hg = clamp(g + highlightBoost)
        const hb = clamp(b + highlightBoost)
        const glowHard = `rgba(${r}, ${g}, ${b}, ${glowHardAlpha})`
        const glowSoft = `rgba(${r}, ${g}, ${b}, ${glowSoftAlpha})`
        el.style.setProperty('--key-color', color)
        el.style.setProperty(
          '--key-glow',
          `radial-gradient(circle at 50% 50%, rgba(${hr}, ${hg}, ${hb}, ${highlightAlpha}) 0%, rgba(${r}, ${g}, ${b}, ${highlightMidAlpha}) 45%, rgba(${r}, ${g}, ${b}, 0) 70%)`
        )
        el.style.setProperty(
          '--key-glow-filter',
          `drop-shadow(0 0 6px ${glowHard}) drop-shadow(0 0 14px ${glowSoft})`
        )
      }
    }

    const underglowBase = 64
    const underglowCount = Math.min(Math.max(totalCount - underglowBase, 0), underglowEls.length)
    for (let i = 0; i < underglowCount; i += 1) {
      const base = (underglowBase + i) * 4
      const { r, g, b, isOff } = applyUnderglowColor(
        data[base],
        data[base + 1],
        data[base + 2],
        data[base + 3]
      )
      const color = isOff ? underglowOffColor : `rgb(${r}, ${g}, ${b})`
      if (color === lastUnderglowColors[i]) {
        continue
      }
      lastUnderglowColors[i] = color
      const el = underglowEls[i]
      if (!el) {
        continue
      }
      el.style.backgroundColor = color
    }
  }

  const getWasmSignature = (response) =>
    response.headers.get('etag') ||
    response.headers.get('last-modified') ||
    response.headers.get('content-length')

  const checkWasmUpdate = async () => {
    try {
      const response = await fetch('/MatrixOSHost.wasm', {
        method: 'HEAD',
        cache: 'no-store'
      })
      if (!response.ok) {
        if (response.status === 404) {
          setWasmMissing('MatrixOS wasm missing.')
        }
        return
      }
      if (isHtmlResponse(response)) {
        setWasmMissing('Matrix OS Image Missing')
        return
      }
      wasmMissing = false
      const signature = getWasmSignature(response)
      if (!signature) {
        return
      }
      if (wasmSignature && signature !== wasmSignature) {
        console.info('MatrixOS wasm update detected, reloading.')
        window.location.reload()
        return
      }
      wasmSignature = signature
    } catch (error) {
    }
  }

  const getCellFromEvent = (event) => {
    if (!grid || !moduleRef) {
      return null
    }
    const rect = grid.getBoundingClientRect()
    const width = moduleRef._MatrixOS_Wasm_GetWidth?.() ?? fallbackSize
    const height = moduleRef._MatrixOS_Wasm_GetHeight?.() ?? fallbackSize
    const x = Math.floor(((event.clientX - rect.left) / rect.width) * width)
    const y = Math.floor(((event.clientY - rect.top) / rect.height) * height)
    if (x < 0 || y < 0 || x >= width || y >= height) {
      return null
    }
    return { x, y }
  }

  const sendKey = (cell, pressed) => {
    if (!moduleRef?._MatrixOS_Wasm_KeyEvent || !cell) {
      return
    }
    moduleRef._MatrixOS_Wasm_KeyEvent(cell.x, cell.y, pressed ? 1 : 0)
  }

  const sendFnKey = (pressed) => {
    if (!moduleRef?._MatrixOS_Wasm_FnEvent) {
      return
    }
    moduleRef._MatrixOS_Wasm_FnEvent(pressed ? 1 : 0)
  }

  const handlePointerDown = (event) => {
    if (!moduleReady || activePointerId !== null || !grid) {
      return
    }
    const cell = getCellFromEvent(event)
    if (!cell) {
      return
    }
    activePointerId = event.pointerId
    activeCell = cell
    grid.setPointerCapture(event.pointerId)
    sendKey(cell, true)
    event.preventDefault()
  }

  const handleFnPointerDown = (event) => {
    if (!moduleReady || fnPointerId !== null) {
      return
    }
    fnPointerId = event.pointerId
    event.currentTarget.setPointerCapture(event.pointerId)
    sendFnKey(true)
    event.preventDefault()
  }

  const handlePointerMove = (event) => {
    if (!moduleReady || activePointerId !== event.pointerId) {
      return
    }
    const cell = getCellFromEvent(event)
    if (!cell) {
      return
    }
    if (!activeCell || cell.x !== activeCell.x || cell.y !== activeCell.y) {
      if (activeCell) {
        sendKey(activeCell, false)
      }
      activeCell = cell
      sendKey(cell, true)
    }
  }

  const endFnPointer = (event) => {
    if (!moduleReady || fnPointerId !== event.pointerId) {
      return
    }
    sendFnKey(false)
    fnPointerId = null
    event.preventDefault()
  }

  const endPointer = (event) => {
    if (!moduleReady || activePointerId !== event.pointerId) {
      return
    }
    if (activeCell) {
      sendKey(activeCell, false)
    }
    activePointerId = null
    activeCell = null
    event.preventDefault()
  }

  const handleReboot = () => {
    console.info('Reboot requested');
    status = 'Rebooting MatrixOS...';
    try {
      window.localStorage.setItem(consolePrefKey, activePanel || '')
    } catch (error) {}

    if (moduleRef?._MatrixOS_Wasm_Reboot) {
      moduleRef._MatrixOS_Wasm_Reboot()
    }
  }

  const waitForRuntime = () => {
    const ready = window.MatrixOSRuntimeReady
    if (ready && typeof ready.then === 'function') {
      return ready
    }

    return new Promise((resolve) => {
      if (!moduleRef) {
        resolve()
        return
      }
      if (moduleRef.runtimeReady) {
        resolve()
        return
      }
      const previous = moduleRef.onRuntimeInitialized
      moduleRef.onRuntimeInitialized = () => {
        if (typeof previous === 'function') {
          previous()
        }
        moduleRef.runtimeReady = true
        resolve()
      }
    })
  }

  onMount(() => {
    checkWasmAvailability()

    moduleRef = window.Module ?? null
    if (!moduleRef) {
      if (!wasmMissing) {
        status = 'MatrixOS wasm not loaded.'
      }
      return () => {}
    }

    const restoreAbort = hookModuleAbort()
    restoreConsole = hookModuleLogging()

    try {
      const stored = window.localStorage.getItem(consolePrefKey)
      if (stored === 'console' || stored === 'settings' || stored === 'connection' || stored === 'version' || stored === 'midi') {
        activePanel = stored
      }
    } catch (error) {}

    checkWasmUpdate()
    reloadTimer = window.setInterval(checkWasmUpdate, 2000)

    const start = () => {
      if (wasmMissing) {
        setTimeout(start, 250)
        return
      }
      if (!moduleRef.runtimeReady && !moduleRef.calledRun) {
        setTimeout(start, 50)
        return
      }
      const heap = moduleRef.HEAPU8 || window.HEAPU8
      if (!heap || !moduleRef._MatrixOS_Wasm_GetFrameBuffer) {
        status = 'Waiting for framebuffer...'
        setTimeout(start, 50)
        return
      }
      moduleReady = true
      status = 'Live framebuffer streaming.'
      lastColors.fill('')
      lastUnderglowColors.fill('')
      if (moduleRef._MatrixOS_Wasm_GetVersionString && moduleRef.UTF8ToString) {
        const ptr = moduleRef._MatrixOS_Wasm_GetVersionString()
        if (ptr) {
          versionLabel = moduleRef.UTF8ToString(ptr)
        }
      }
      renderFrame()
    }

    waitForRuntime().then(start)

    return () => {
      if (rafId) {
        cancelAnimationFrame(rafId)
      }
      if (reloadTimer) {
        clearInterval(reloadTimer)
      }
      if (restoreAbort) {
        restoreAbort()
      }
      if (restoreConsole) {
        restoreConsole()
      }
    }
  })
</script>

<svelte:head>
  <title>Matrix OS Simulator</title>
</svelte:head>

<main class="app-shell">
  <header class="app-header glass">
    <div class="brand">
      <picture class="brand-logo">
        <source srcset="/203dark.svg" media="(prefers-color-scheme: dark)" />
        <img src="/203.svg" alt="203 Systems" />
      </picture>
      <div class="brand-text">
        <div class="brand-title">Matrix OS Simulator</div>
      </div>
    </div>
    <div class="header-actions">
      <button
        class="app-btn"
        type="button"
        class:active-btn={activePanel === 'version'}
        on:click={() => (activePanel = activePanel === 'version' ? null : 'version')}
      >
        <span class="btn-label">Matrix OS {versionLabel}</span>
      </button>
    </div>
  </header>

  <section class="app-main" class:panel-open={!!activePanel}>
    <div class="app-stage glass">
      <div class="visualizer-shell" class:status-active={!moduleReady}>
        <div class="lp">
          <div class="lp-underglow" aria-hidden="true">
            <div class="lp-underglow-row">
              {#each edgeSlots as _, x}
                <div class="lp-underglow-led-parent">
                  <div class="lp-underglow-led" bind:this={underglowEls[getUnderglowIndex(x, -1)]}></div>
                </div>
              {/each}
            </div>

            <div class="lp-underglow-middle">
              <div class="lp-underglow-column">
                {#each edgeSlots as _, y}
                  <div class="lp-underglow-led-parent">
                    <div class="lp-underglow-led" bind:this={underglowEls[getUnderglowIndex(-1, y)]}></div>
                  </div>
                {/each}
              </div>

              <div class="lp-underglow-column">
                {#each edgeSlots as _, y}
                  <div class="lp-underglow-led-parent">
                    <div class="lp-underglow-led" bind:this={underglowEls[getUnderglowIndex(8, y)]}></div>
                  </div>
                {/each}
              </div>
            </div>

            <div class="lp-underglow-row">
              {#each edgeSlots as _, x}
                <div class="lp-underglow-led-parent">
                  <div class="lp-underglow-led" bind:this={underglowEls[getUnderglowIndex(x, 8)]}></div>
                </div>
              {/each}
            </div>
          </div>

          <div class="lp-border">
            <div
              class="lp-controls"
              bind:this={grid}
              on:pointerdown={handlePointerDown}
              on:pointermove={handlePointerMove}
              on:pointerup={endPointer}
              on:pointercancel={endPointer}
              on:pointerleave={endPointer}
            >
              {#each edgeSlots as _, y}
                <div class="lp-controls-row">
                  {#each edgeSlots as _, x}
                    <div
                      class="lp-btn-parent"
                      style={getCornerClip(x, y) ? `clip-path: ${getCornerClip(x, y)};` : ''}
                    >
                      <div
                        class="lp-normal-btn"
                        bind:this={keypadEls[y * 8 + x]}
                      ></div>
                    </div>
                  {/each}
                </div>
              {/each}
            </div>
          </div>

          <div
            class="lp-center-key"
            role="button"
            aria-label="Function key"
            on:pointerdown={handleFnPointerDown}
            on:pointerup={endFnPointer}
            on:pointercancel={endFnPointer}
            on:pointerleave={endFnPointer}
          ></div>
        </div>
        {#if !moduleReady}
          <div class="visualizer-overlay" aria-live="polite">
            <div class="status-card" class:status-error={wasmMissing}>{status}</div>
          </div>
        {/if}
      </div>
    </div>

    {#if activePanel}
      <aside class="console-panel glass" aria-label="Side panel">
        {#if activePanel === 'console'}
          <div class="panel-content">
            <div class="console-header">
              <div class="console-title console-title-large">Console Log</div>
            </div>
            <div class="console-body-wrapper empty-panel-wrapper">
              <div
                class="console-body"
                bind:this={consoleBody}
                on:scroll={() => {
                  if (!consoleBody) return
                  const nearBottom = consoleBody.scrollTop + consoleBody.clientHeight >= consoleBody.scrollHeight - 20
                  autoScroll = nearBottom
                  userScrolledUp = !nearBottom
                }}
              >
                {#if consoleMessages.length === 0}
                  <div class="console-empty empty-center">No logs yet.</div>
                {:else}
                  {#each consoleMessages as log (log.id)}
                    <div class={`console-line level-${log.level}`}>
                      <span class="console-text" style={log.color ? `color:${log.color}` : ''}>{log.text}</span>
                    </div>
                  {/each}
                {/if}
              </div>
              {#if userScrolledUp}
                <button
                  class="console-scroll-bottom"
                  type="button"
                  on:click={() => {
                    if (consoleBody) {
                      consoleBody.scrollTop = consoleBody.scrollHeight
                    }
                    autoScroll = true
                    userScrolledUp = false
                  }}
                >
                  v
                </button>
              {/if}
            </div>
          </div>
        {:else if activePanel === 'settings'}
          <div class="panel-content">
            <div class="console-header">
              <div class="console-title console-title-large">Settings</div>
            </div>
            <div class="console-body-wrapper empty-panel-wrapper">
              <div class="console-body settings-panel empty-panel">
                <div class="console-empty empty-center">No settings currently.</div>
              </div>
            </div>
          </div>
        {:else if activePanel === 'connection'}
          <div class="panel-content">
            <div class="console-header">
              <div class="console-title console-title-large">Connection</div>
            </div>
            <div class="console-body-wrapper empty-panel-wrapper">
              <div class="console-body settings-panel empty-panel">
                <div class="console-empty empty-center">No settings currently.</div>
              </div>
            </div>
          </div>
        {:else if activePanel === 'version'}
          <div class="panel-content">
            <div class="console-header">
              <div class="console-title console-title-large">OS Version</div>
            </div>
            <div class="console-body-wrapper empty-panel-wrapper">
              <div class="console-body settings-panel empty-panel">
                <div class="console-empty empty-center">Under construction.</div>
              </div>
            </div>
          </div>
        {:else if activePanel === 'midi'}
          <div class="panel-content">
            <div class="console-header">
              <div class="console-title console-title-large">MIDI Monitor</div>
            </div>
            <div class="console-body-wrapper empty-panel-wrapper">
              <div class="console-body settings-panel empty-panel">
                <div class="console-empty empty-center">Under construction.</div>
              </div>
            </div>
          </div>
        {/if}
      </aside>
    {/if}
  </section>

  <footer class="app-footer glass">
    <nav class="app-actions" aria-label="Matrix OS controls">
      <button
        class="app-btn"
        type="button"
        class:active-btn={activePanel === 'settings'}
        on:click={() => (activePanel = activePanel === 'settings' ? null : 'settings')}
      >
        <Settings size={18} class="btn-icon" />
        <span class="btn-label">Setting</span>
      </button>
      <button
        class="app-btn"
        type="button"
        class:active-btn={activePanel === 'connection'}
        on:click={() => (activePanel = activePanel === 'connection' ? null : 'connection')}
      >
        <ArrowsHorizontal size={18} class="btn-icon" />
        <span class="btn-label">Connection</span>
      </button>
      <button class="app-btn" type="button" on:click={handleReboot}>
        <Restart size={18} class="btn-icon" />
        <span class="btn-label">Reboot</span>
      </button>
      <button
        class="app-btn"
        type="button"
        class:active-btn={activePanel === 'console'}
        on:click={() => (activePanel = activePanel === 'console' ? null : 'console')}
      >
        <Terminal size={18} class="btn-icon" />
        <span class="btn-label">Console Log</span>
      </button>
      <button
        class="app-btn"
        type="button"
        class:active-btn={activePanel === 'midi'}
        on:click={() => (activePanel = activePanel === 'midi' ? null : 'midi')}
      >
        <Music size={18} class="btn-icon" />
        <span class="btn-label">MIDI Monitor</span>
      </button>
      <a class="app-btn" href="https://discord.gg/rRVCBHHPfw" target="_blank" rel="noreferrer noopener">
        <LogoDiscord size={18} class="btn-icon" />
        <span class="btn-label">Discord</span>
      </a>
      <a class="app-btn" href="https://matrix.203.io" target="_blank" rel="noreferrer noopener">
        <Help size={18} class="btn-icon" />
        <span class="btn-label">Wiki</span>
      </a>
    </nav>
  </footer>
</main>
