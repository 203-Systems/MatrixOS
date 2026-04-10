# MystrixSIL Tool TODO

## Phase 1: Runtime Stability
- [x] MystrixSIL boots reliably in browser
- [x] Shell launches reliably after boot
- [ ] Grid input works after boot
- [ ] Function key works after boot
- [x] Reboot actually resets the emulator
- [x] Bootloader action behaves consistently
- [x] No `table index is out of bounds` runtime crash remains

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

## Phase 4b: Input Tooling — Runtime Wiring (Round 2)
- [x] Add WASM exports for runtime keypad state (GetKeypadState, GetKeypadStateLength, GetFnState)
- [x] Poll runtime state from DevicePanel render loop
- [x] Show "Runtime" state section in InputPanel reflecting what the OS sees
- [x] Add runtime-side state stores (runtimeGridKeys, runtimeFnActive)

## Phase 5: Logs and Runtime (Round 1)
- [x] Add structured logs panel (LogsPanel.svelte — ANSI parsing, color, level tags)
- [x] Add filtering/search for logs (text filter + level dropdown)
- [x] Add runtime/app-state panel (RuntimePanel.svelte — status/health/connection cards)
- [x] Show boot/runtime phase (runtime status card)
- [x] Show current app lifecycle state (uptime, rotation via new WASM exports)

## Phase 5b: Logs Improvements (Round 2)
- [x] Remove suppression of RuntimeError / worker-error messages from log store
- [x] All WASM stderr now surfaces in Logs panel for runtime debugging

## Phase 6: Architecture Cleanup (Round 1)
- [x] Refactor frontend into cleaner stores/view-model structure (wasm.js, logs.js, input.js)
- [x] Remove ad hoc page logic where practical (monolithic App.svelte → 6 components)
- [x] Keep MystrixSIL-specific hooks device-local when possible
- [x] Keep OS-layer changes minimal (zero OS-layer changes)
- [x] Add GetRotation/GetUptimeMs WASM exports to Device.cpp

## Phase 6b: Architecture Cleanup (Round 2)
- [x] Fix EXPORTED_FUNCTIONS to include GetRotation, GetUptimeMs (added in R1 but missing from export list)
- [x] Add GetKeypadState, GetKeypadStateLength, GetFnState WASM exports

## Phase 7: Layout and Tool Model Refactor (Round 4)
- [x] Convert layout to: left global nav → center workspace → right ToolTray + panel stack
- [x] Left nav now shows Device / Settings / Firmware (global product navigation)
- [x] Move Input / Logs / Runtime out of left nav into right-side tool panels
- [x] Add ToolTray icon bar on right edge (toggleable, 8 tool slots)
- [x] Add ToolPanelStack: vertically stacked, closable tool panels
- [x] Add panel shells for UI, MIDI, HID, Serial, Usage
- [x] Merge RAM/tasks concepts into Usage panel
- [x] Add Settings page placeholder
- [x] Add Firmware page placeholder
- [x] Remove DFU button from top bar
- [x] Rename Reboot to Reset
- [x] Show full build identity: channel + git hash + dirty/clean state
- [x] Update Device.cpp to expose MATRIXOS_VERSION_STRING via WASM
- [x] Inject git hash/dirty via Vite build-time defines
- [x] Web UI Vite build succeeds (`npm run build` clean)

## Phase 8: Forward-Looking Hooks
- [ ] Leave a clean path for MIDI tooling
- [ ] Leave a clean path for scenarios/replay
- [ ] Leave a clean path for SDK/debug transport
- [ ] Leave a clean path for VS Code / GDB-adjacent tooling

## Validation (Round 1)
- [x] Web UI Vite build succeeds (`npm run build` clean)
- [ ] Emscripten WASM build succeeds
- [x] Web UI starts locally
- [ ] Dashboard loads without major runtime errors
- [ ] Input interactions are visible in the UI
- [x] Device controls work from the UI (grid + FN + reboot + DFU)

## Validation (Round 2)
- [x] Web UI Vite build succeeds after all Round 2 changes
- [x] FreeRTOS stub vTaskDelete now properly terminates self-deleting tasks (longjmp)
- [x] FreeRTOS stub taskYIELD/vTaskDelay check deleted/suspended flags
- [x] All WASM exports in Device.cpp present in EXPORTED_FUNCTIONS list
- [ ] Emscripten WASM build succeeds (needs Emscripten SDK on this machine)
- [ ] Browser validation: Shell launches without crash
- [ ] Browser validation: Input panel shows runtime state

## Validation (Round 3)
- [x] All `std::function`-based UI callbacks replaced with `UICallback<Sig>` (named template trampolines)
- [x] Added `-sALLOW_TABLE_GROWTH=1` Emscripten linker flag
- [x] Emscripten WASM build succeeds (all .o files + link to MatrixOSHost.js)
- [ ] Browser validation: Shell launches without crash
- [ ] Browser validation: Application Launcher UI is functional
