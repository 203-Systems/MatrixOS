# MystrixSIL Tool TODO

## Phase 1: Runtime Stability
- [x] MystrixSIL boots reliably in browser
- [ ] Shell launches reliably after boot
- [ ] Grid input works after boot
- [ ] Function key works after boot
- [x] Reboot actually resets the emulator
- [x] Bootloader action behaves consistently
- [ ] No `table index is out of bounds` runtime crash remains

## Phase 2: Dashboard Shell (Round 1)
- [x] Replace demo-style layout with a professional SaaS-style dashboard shell
- [x] Add top status bar (TopBar.svelte — status dot, version, err/warn badges, reboot/DFU buttons)
- [x] Add left navigation (LeftNav.svelte — collapsible icon sidebar, 4 sections)
- [x] Add main workspace with section switching (App.svelte shell, localStorage persistence)
- [x] Flat/dense/professional CSS (app.css rewrite — no glass/gradient, pure dashboard vars)

## Phase 3: Device Panel (Round 1)
- [x] Show 8×8 device grid (DevicePanel.svelte — extracted from old monolith)
- [x] Show function key (center FN diamond with pointer events)
- [x] Show LED/framebuffer state (requestAnimationFrame render loop, RGBW → CSS)
- [x] Show underglow ring (32-LED perimeter)
- [x] Show current app and runtime status (overlay when not ready)
- [x] Expose reboot / bootloader actions clearly (TopBar)

## Phase 4: Input Tooling (Round 1)
- [x] Add live input event log (InputPanel.svelte — timestamped event rows)
- [x] Add current input state/snapshot view (active key chips)
- [x] Add manual grid key injection controls (8×8 mini-grid)
- [x] Add manual function-key injection controls (FN button)
- [x] Add clear event ordering/timestamps
- [x] Log raw injected input and runtime-side input events

## Phase 5: Logs and Runtime (Round 1)
- [x] Add structured logs panel (LogsPanel.svelte — ANSI parsing, color, level tags)
- [x] Add filtering/search for logs (text filter + level dropdown)
- [x] Add runtime/app-state panel (RuntimePanel.svelte — status/health/connection cards)
- [x] Show boot/runtime phase (runtime status card)
- [x] Show current app lifecycle state (uptime, rotation via new WASM exports)
- [x] Increase log information density without losing readability
- [ ] Keep browser/worker diagnostics out of the MatrixOS log stream

## Phase 6: Architecture Cleanup (Round 1)
- [x] Refactor frontend into cleaner stores/view-model structure (wasm.js, logs.js, input.js)
- [x] Remove ad hoc page logic where practical (monolithic App.svelte → 6 components)
- [x] Keep MystrixSIL-specific hooks device-local when possible
- [x] Keep OS-layer changes minimal (zero OS-layer changes)
- [x] Add GetRotation/GetUptimeMs WASM exports to Device.cpp

## Phase 7: Forward-Looking Hooks
- [ ] Leave a clean path for MIDI tooling
- [ ] Leave a clean path for scenarios/replay
- [ ] Leave a clean path for SDK/debug transport
- [ ] Leave a clean path for VS Code / GDB-adjacent tooling

## Phase 8: Layout Refinement
- [x] Make right-side tool panels stack horizontally instead of vertically
- [x] Keep the device workspace wide enough when multiple tools are open
- [x] Percentage-based default widths (30% / 50% / 60% / 68% cap)
- [x] User override with smart release on tool count change
- [x] Window resize re-clamp
- [x] Min 420px center workspace, min 220px per tool column

## Phase 9: Tool Panel Expansion
- [x] Add Application panel (live runtime status, version, rotation, uptime)
- [x] Add USB panel (placeholder shell)
- [x] Add Gyro panel (placeholder shell)
- [x] Add Battery panel (placeholder shell)
- [x] Add Storage panel (placeholder shell)
- [x] Register all 13 tools in deviceTools registry
- [x] Add tray icons for all new tools

## Phase 10: UI Scale & Branding
- [x] Root font-size 14→15px (~120% density increase)
- [x] TopBar height 42→50px, title font 0.85→0.96rem
- [x] LeftNav width 56→60px, hover 140→160px, icon 18→22
- [x] ToolTray width 40→48px, hover 110→130px, icon 16→20
- [x] Panel headers padding 4px 8px → 6px 10px, font 0.72→0.8rem
- [x] Product name: "MystrixSIL" → "Matrix OS Developer Toolkit"
- [x] Page title updated

## Validation (Round 1)
- [x] Web UI Vite build succeeds (`npm run build` clean)
- [ ] Emscripten WASM build succeeds
- [x] Web UI starts locally
- [ ] Dashboard loads without major runtime errors
- [ ] Input interactions are visible in the UI
- [x] Device controls work from the UI (grid + FN + reboot + DFU)
