<script>
  import { onMount } from 'svelte'
  import { get } from 'svelte/store'
  import { moduleRef, moduleReady, wasmMissing, runtimeStatus, sendGridKey, sendFnKey, sendTouchBarKey, tickKeypad, getRuntimeKeypadState, getRuntimeFnState } from '../stores/wasm.js'
  import { logInputEvent, pollRuntimeState } from '../stores/input.js'

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

  let grid
  let keypadEls = Array(64).fill(null)
  let lastColors = Array(64).fill('')
  let underglowEls = Array(32).fill(null)
  let lastUnderglowColors = Array(32).fill('')
  let activePointerId = null
  let fnPointerId = null
  let activeCell = null
  let rafId = 0

  const clamp = (v) => Math.max(0, Math.min(255, v))
  const overlayChannel = (v) => Math.round(baseOverlay + (255 - baseOverlay) * (clamp(v) / 255))

  const applyLedColor = (r, g, b, w) => {
    let rr = clamp(r + w), gg = clamp(g + w), bb = clamp(b + w)
    const isOff = rr === 0 && gg === 0 && bb === 0
    if (!isOff) { rr = overlayChannel(rr); gg = overlayChannel(gg); bb = overlayChannel(bb) }
    return { r: rr, g: gg, b: bb, isOff }
  }

  const applyUnderglowColor = (r, g, b, w) => {
    const rr = clamp(r + w), gg = clamp(g + w), bb = clamp(b + w)
    return { r: rr, g: gg, b: bb, isOff: rr === 0 && gg === 0 && bb === 0 }
  }

  const getCornerClip = (x, y) => {
    switch (x + y * 10) {
      case 43: return 'polygon(80% 0, 100% 20%, 100% 100%, 0 100%, 0 0)'
      case 44: return 'polygon(20% 0, 100% 0, 100% 100%, 0 100%, 0 20%)'
      case 33: return 'polygon(100% 0, 100% 80%, 80% 100%, 0 100%, 0 0)'
      case 34: return 'polygon(100% 0, 100% 100%, 20% 100%, 0 80%, 0 0)'
      default: return ''
    }
  }

  const getUnderglowIndex = (x, y) => {
    if (x === 8 && y >= 0 && y < 8) return 7 - y
    if (y === -1 && x >= 0 && x < 8) return 8 + (7 - x)
    if (x === -1 && y >= 0 && y < 8) return 16 + y
    if (y === 8 && x >= 0 && x < 8) return 24 + x
    return 0
  }

  function renderFrame() {
    rafId = requestAnimationFrame(renderFrame)
    const mod = get(moduleRef)
    const ready = get(moduleReady)
    if (!ready || !mod || !mod.calledRun) return

    if (mod._MatrixOS_Wasm_KeypadTick && (activePointerId !== null || fnPointerId !== null)) {
      mod._MatrixOS_Wasm_KeypadTick()
    }

    const heap = mod.HEAPU8 || window.HEAPU8
    if (!heap || !mod._MatrixOS_Wasm_GetFrameBuffer) return

    const width = mod._MatrixOS_Wasm_GetWidth?.() ?? fallbackSize
    const height = mod._MatrixOS_Wasm_GetHeight?.() ?? fallbackSize
    const byteLength = mod._MatrixOS_Wasm_GetFrameBufferByteLength?.() ?? 0
    const ptr = mod._MatrixOS_Wasm_GetFrameBuffer?.() ?? 0
    if (!ptr || !byteLength) return

    const data = heap.subarray(ptr, ptr + byteLength)
    const totalCount = Math.floor(byteLength / 4)
    const gridCount = Math.min(width * height, keypadEls.length, totalCount)

    for (let i = 0; i < gridCount; i++) {
      const base = i * 4
      const { r, g, b, isOff } = applyLedColor(data[base], data[base + 1], data[base + 2], data[base + 3])
      const color = isOff ? offColor : `rgb(${r}, ${g}, ${b})`
      if (color === lastColors[i]) continue
      lastColors[i] = color
      const el = keypadEls[i]
      if (!el) continue
      if (isOff) {
        el.style.setProperty('--key-color', offColor)
        el.style.removeProperty('--key-glow')
        el.style.removeProperty('--key-glow-filter')
      } else {
        const hr = clamp(r + highlightBoost), hg = clamp(g + highlightBoost), hb = clamp(b + highlightBoost)
        el.style.setProperty('--key-color', color)
        el.style.setProperty('--key-glow',
          `radial-gradient(circle at 50% 50%, rgba(${hr},${hg},${hb},${highlightAlpha}) 0%, rgba(${r},${g},${b},${highlightMidAlpha}) 45%, rgba(${r},${g},${b},0) 70%)`)
        el.style.setProperty('--key-glow-filter',
          `drop-shadow(0 0 6px rgba(${r},${g},${b},${glowHardAlpha})) drop-shadow(0 0 14px rgba(${r},${g},${b},${glowSoftAlpha}))`)
      }
    }

    const ugBase = 64
    const ugCount = Math.min(Math.max(totalCount - ugBase, 0), underglowEls.length)
    for (let i = 0; i < ugCount; i++) {
      const base = (ugBase + i) * 4
      const { r, g, b, isOff } = applyUnderglowColor(data[base], data[base + 1], data[base + 2], data[base + 3])
      const color = isOff ? underglowOffColor : `rgb(${r}, ${g}, ${b})`
      if (color === lastUnderglowColors[i]) continue
      lastUnderglowColors[i] = color
      const el = underglowEls[i]
      if (el) el.style.backgroundColor = color
    }

    // Poll runtime-side keypad state for the Input panel
    pollRuntimeState(getRuntimeKeypadState(), getRuntimeFnState())
  }

  const getCellFromEvent = (event) => {
    if (!grid) return null
    const mod = get(moduleRef)
    if (!mod) return null
    const rect = grid.getBoundingClientRect()
    const width = mod._MatrixOS_Wasm_GetWidth?.() ?? fallbackSize
    const height = mod._MatrixOS_Wasm_GetHeight?.() ?? fallbackSize
    const x = Math.floor(((event.clientX - rect.left) / rect.width) * width)
    const y = Math.floor(((event.clientY - rect.top) / rect.height) * height)
    if (x < 0 || y < 0 || x >= width || y >= height) return null
    return { x, y }
  }

  const handlePointerDown = (event) => {
    if (!get(moduleReady) || activePointerId !== null || !grid) return
    const cell = getCellFromEvent(event)
    if (!cell) return
    activePointerId = event.pointerId
    activeCell = cell
    grid.setPointerCapture(event.pointerId)
    sendGridKey(cell.x, cell.y, true)
    logInputEvent('grid', cell.x, cell.y, true)
    event.preventDefault()
  }

  const handlePointerMove = (event) => {
    if (!get(moduleReady) || activePointerId !== event.pointerId) return
    const cell = getCellFromEvent(event)
    if (!cell) return
    if (!activeCell || cell.x !== activeCell.x || cell.y !== activeCell.y) {
      if (activeCell) {
        sendGridKey(activeCell.x, activeCell.y, false)
        logInputEvent('grid', activeCell.x, activeCell.y, false)
      }
      activeCell = cell
      sendGridKey(cell.x, cell.y, true)
      logInputEvent('grid', cell.x, cell.y, true)
    }
  }

  const endPointer = (event) => {
    if (!get(moduleReady) || activePointerId !== event.pointerId) return
    if (activeCell) {
      sendGridKey(activeCell.x, activeCell.y, false)
      logInputEvent('grid', activeCell.x, activeCell.y, false)
    }
    activePointerId = null
    activeCell = null
    event.preventDefault()
  }

  const handleFnPointerDown = (event) => {
    if (!get(moduleReady) || fnPointerId !== null) return
    fnPointerId = event.pointerId
    event.currentTarget.setPointerCapture(event.pointerId)
    sendFnKey(true)
    logInputEvent('fn', 0, 0, true)
    event.preventDefault()
  }

  const endFnPointer = (event) => {
    if (!get(moduleReady) || fnPointerId !== event.pointerId) return
    sendFnKey(false)
    logInputEvent('fn', 0, 0, false)
    fnPointerId = null
    event.preventDefault()
  }

  // Touchbar pointer tracking: Map<pointerId, {side, index}>
  const touchbarPointers = new Map()

  const handleTouchBarPointerDown = (event, side, index) => {
    if (!get(moduleReady)) return
    touchbarPointers.set(event.pointerId, { side, index })
    event.currentTarget.setPointerCapture(event.pointerId)
    sendTouchBarKey(side, index, true)
    logInputEvent('touchbar', side, index, true)
    event.preventDefault()
  }

  const handleTouchBarPointerUp = (event) => {
    const info = touchbarPointers.get(event.pointerId)
    if (!info) return
    touchbarPointers.delete(event.pointerId)
    sendTouchBarKey(info.side, info.index, false)
    logInputEvent('touchbar', info.side, info.index, false)
    event.preventDefault()
  }

  onMount(() => {
    lastColors.fill('')
    lastUnderglowColors.fill('')
    renderFrame()
    return () => {
      if (rafId) cancelAnimationFrame(rafId)
    }
  })
</script>

<div class="device-panel">
  <div class="device-grid-area">
    <div class="device-vis" class:vis-inactive={!$moduleReady}>
      <div class="lp-outer">
        <!-- Left touchbar -->
        <div class="lp-touchbar lp-touchbar-left">
          {#each edgeSlots as _, i}
            <div
              class="lp-tb-btn"
              role="button"
              aria-label={`Left touchbar ${i}`}
              on:pointerdown={(e) => handleTouchBarPointerDown(e, 0, i)}
              on:pointerup={handleTouchBarPointerUp}
              on:pointercancel={handleTouchBarPointerUp}
              on:pointerleave={handleTouchBarPointerUp}
            ></div>
          {/each}
        </div>

        <div class="lp">
          <!-- Underglow ring -->
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

        <!-- Grid border and buttons -->
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
                    <div class="lp-normal-btn" bind:this={keypadEls[y * 8 + x]}></div>
                  </div>
                {/each}
              </div>
            {/each}
          </div>
        </div>

        <!-- FN center key -->
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

      <!-- Right touchbar -->
      <div class="lp-touchbar lp-touchbar-right">
        {#each edgeSlots as _, i}
          <div
            class="lp-tb-btn"
            role="button"
            aria-label={`Right touchbar ${i}`}
            on:pointerdown={(e) => handleTouchBarPointerDown(e, 1, i)}
            on:pointerup={handleTouchBarPointerUp}
            on:pointercancel={handleTouchBarPointerUp}
            on:pointerleave={handleTouchBarPointerUp}
          ></div>
        {/each}
      </div>

    </div><!-- end lp-outer -->

      {#if !$moduleReady}
        <div class="vis-overlay" aria-live="polite">
          <div class="vis-status" class:vis-error={$wasmMissing}>{$runtimeStatus}</div>
        </div>
      {/if}
    </div><!-- end device-vis -->
  </div><!-- end device-grid-area -->
</div><!-- end device-panel -->

<style>
  .device-panel {
    display: flex;
    flex-direction: column;
    height: 100%;
    gap: 12px;
    padding: 16px;
    overflow: auto;
  }
  .device-grid-area {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    min-height: 0;
  }
  .device-vis {
    position: relative;
    display: flex;
    align-items: center;
    justify-content: center;
    width: 100%;
  }
  .vis-inactive .lp {
    filter: blur(4px) saturate(0.8);
  }
  .vis-overlay {
    position: absolute;
    inset: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 5;
  }
  .vis-status {
    padding: 6px 14px;
    border-radius: 6px;
    background: rgba(14, 14, 18, 0.85);
    border: 1px solid var(--border);
    color: var(--text);
    font-size: 0.8rem;
    letter-spacing: 0.06em;
    text-transform: uppercase;
  }
  .vis-error {
    border-color: rgba(255, 107, 107, 0.5);
    color: #ffd2d2;
    background: rgba(36, 16, 18, 0.9);
  }

  /* Grid / LED visualization */
  .lp-outer {
    display: flex;
    flex-direction: row;
    align-items: center;
    gap: 6px;
    justify-content: center;
    width: 100%;
    max-width: 520px;
  }

  .lp-touchbar {
    display: flex;
    flex-direction: column;
    gap: 2%;
    flex-shrink: 0;
    align-self: stretch;
    padding: 2% 0;
    box-sizing: border-box;
  }

  .lp-tb-btn {
    flex: 1;
    width: 10px;
    border-radius: 9999px;
    background-color: rgba(255, 255, 255, 0.08);
    cursor: pointer;
    transition: background-color 0.15s ease, transform 0.15s ease;
    touch-action: none;
    user-select: none;
    transform: scale(0.85);
  }

  .lp-tb-btn:hover {
    background-color: rgba(255, 255, 255, 0.28);
    transform: scale(1.0);
  }

  .lp-tb-btn:active {
    background-color: rgba(255, 255, 255, 0.55);
    transform: scale(1.05);
  }

  .lp {
    position: relative;
    flex: 1;
    max-width: 480px;
    aspect-ratio: 1 / 1;
    z-index: 1;
  }
  .lp-border {
    background-color: var(--device-body, #151518);
    border: 2px solid var(--device-border, #2f2f36);
    border-radius: 3%;
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.4);
    position: absolute;
    width: 100%;
    aspect-ratio: 1 / 1;
    z-index: 2;
  }
  .lp-underglow {
    position: absolute;
    top: -3%; left: -3%;
    height: 106%; width: 106%;
    filter: blur(20px) saturate(200%) brightness(200%);
    z-index: 1;
    pointer-events: none;
  }
  .lp-underglow-row {
    height: 6%;
    display: flex;
    padding-left: 6%; padding-right: 6%;
    gap: 1.5%;
  }
  .lp-underglow-middle {
    display: flex;
    height: 88%;
    justify-content: space-between;
  }
  .lp-underglow-column {
    height: 100%; width: 6%;
    display: flex;
    flex-direction: column;
    gap: 1.5%;
    padding: 0.5% 0;
  }
  .lp-underglow-row .lp-underglow-led-parent,
  .lp-underglow-column .lp-underglow-led-parent {
    width: 100%;
    display: flex;
    justify-content: center;
    align-items: center;
  }
  .lp-underglow-column .lp-underglow-led-parent {
    height: 20%;
  }
  .lp-underglow-led {
    height: 100%; width: 100%;
    background-color: rgba(255, 255, 255, 0);
  }
  .lp-controls {
    position: relative;
    height: 100%; width: 100%;
    display: flex;
    gap: 1.5%;
    flex-direction: column;
    padding: 3%;
    z-index: 10;
    touch-action: none;
  }
  .lp-controls-row {
    height: 100%;
    display: flex;
    gap: 1.5%;
  }
  .lp-btn-parent {
    height: 100%; width: 100%;
    display: flex;
    justify-content: center;
    align-items: center;
    filter: var(--key-glow-filter, none);
  }
  .lp-normal-btn {
    padding: 0; border: none;
    height: 100%; width: 100%;
    border-radius: 10%;
    background-color: var(--key-color, var(--device-button, #5b5b63));
    background-image: var(--key-glow, none);
    pointer-events: none;
    background-repeat: no-repeat;
    background-position: center;
    background-size: 100% 100%;
  }
  .lp-center-key {
    position: absolute;
    height: 3.2%; width: 3.2%;
    z-index: 3;
    background-color: rgba(255, 255, 255, 0.14);
    top: 50%; left: 50%;
    transform: translate(-50%, -50%) rotate(45deg);
    border-radius: 20%;
    box-shadow: 0 0 6px rgba(0, 0, 0, 0.45), 0 0 14px rgba(255, 255, 255, 0.22);
    pointer-events: auto;
    touch-action: none;
    cursor: pointer;
  }
  .lp-center-key:hover {
    background-color: rgba(255, 255, 255, 0.28);
    box-shadow: 0 0 8px rgba(255, 255, 255, 0.45);
  }
</style>
