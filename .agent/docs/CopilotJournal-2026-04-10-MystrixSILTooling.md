# Copilot Journal: MystrixSIL Tooling

Date: 2026-04-10

## Goal
Use Copilot CLI as an execution subagent to turn MystrixSIL from a browser demo into a more serious SaaS-style debugging dashboard, while Codex reviews each round and validates the runtime in the browser.

## Context
- The tool direction is defined in `.agent/docs/MystrixSILToolSpec.md`
- The current execution checklist is tracked in `.agent/docs/MystrixSILToolTODO.md`
- The active implementation target is the MystrixSIL worktree, not the main repo root

## Round 1 Plan
- build a real dashboard shell
- improve device/input/log/runtime visibility
- keep OS-layer changes minimal
- preserve current OS4.0 input model

## Copilot Command Shape
```powershell
python .agent/tools/copilot_prompt.py --cwd <worktree> --prompt "Read .agent/docs/MystrixSILToolSpec.md, .agent/docs/MystrixSILToolTODO.md, and .agent/tools/prompts/mystrixsil_tool_round_1.txt, then execute the task exactly. Work from the current working tree state and do not restart from scratch."
```

## Review Notes
- Round 1 did produce a real dashboard shell and the page now loads in the browser.
- The layout is directionally correct:
  - top bar
  - left nav
  - device/input/logs/runtime sections
  - split frontend stores/components
- However, runtime stability is still not good enough.
- Direct browser validation still reproduces:
  - `RuntimeError: table index is out of bounds`
- The failure now appears after boot skip / Shell launch path, after:
  - `UI: Register UI Application Launcher`
- So the runtime-stability items in the TODO are still open and must be corrected in the next round.

## Round 2 Plan
- prioritize runtime stability over additional styling
- remove the remaining `table index is out of bounds` crash
- make the Input panel reflect real runtime-side events/state, not just frontend injection
- keep the dashboard shell from round 1, but make it honest and usable

## Round 2 Results

### Root Cause Analysis
The `RuntimeError: table index is out of bounds` crash was caused by a fundamental flaw in the FreeRTOS stub's `vTaskDelete()` implementation. On real FreeRTOS, `vTaskDelete(NULL)` (self-deletion) **never returns** — the scheduler removes the task and context-switches away. But the WASM stub simply set two flags (`deleted=true`, `running=false`) and returned normally.

This meant:
1. Boot animation calls `Exit()` → `ExitAPP()` → destructs the app object → calls `vTaskDelete(activeAppTask)`
2. `vTaskDelete` returned to `ExitAPP`, which returned to `Application::Start()`'s `while(true)` loop
3. The loop called `Loop()` again on the already-destroyed application object
4. Virtual dispatch through the corrupted vtable produced an invalid WASM indirect call table index
5. → `RuntimeError: table index is out of bounds`

Additionally, `taskYIELD()` and `vTaskDelay()` never checked for external deletion/suspension, meaning the Supervisor's force-exit path (calling `vTaskDelete` on another task) also couldn't stop that task.

### Fix: FreeRTOS Stub Task Lifecycle (`FreeRTOSStub.cpp`)
- Added `setjmp`/`longjmp` mechanism to `TaskControl` and `xTaskCreate`
- `vTaskDelete(self)` now `longjmp`s back to the `setjmp` in the task creation lambda, cleanly terminating the thread
- `taskYIELD` and `vTaskDelay` now check `deleted`/`suspended` flags and terminate or block accordingly
- `vTaskSuspend` longjmps if deletion occurs during suspension
- `ulTaskNotifyTake` terminates if woken by deletion
- External `vTaskDelete` now notifies suspend/notify condition variables so the target thread wakes up and can observe the deleted flag

### Fix: Missing WASM Exports (`CMakeLists.txt`)
- Added `_MatrixOS_Wasm_GetRotation` and `_MatrixOS_Wasm_GetUptimeMs` to `EXPORTED_FUNCTIONS` (added in Device.cpp in Round 1 but never exported)
- Added new exports: `_MatrixOS_Wasm_GetKeypadState`, `_MatrixOS_Wasm_GetKeypadStateLength`, `_MatrixOS_Wasm_GetFnState`

### Input Panel → Runtime Wiring
- Added three new WASM exports in Device.cpp: `GetKeypadState` (returns pointer to active-state array), `GetKeypadStateLength`, `GetFnState`
- Added `getRuntimeKeypadState()` and `getRuntimeFnState()` to `wasm.js` store
- Added `runtimeGridKeys` / `runtimeFnActive` stores to `input.js`, polled via `pollRuntimeState()`
- DevicePanel's `renderFrame()` loop now polls runtime keypad state every animation frame
- InputPanel shows a "Runtime" state section (green chips) alongside the existing "Injected" section

### Logs Panel Improvements
- Removed the suppression of `RuntimeError: table index is out of bounds` and `worker sent an error!` messages from `logs.js`
- All WASM stderr output now surfaces in the Logs panel for runtime debugging

### Files Changed
- `Devices/MystrixSIL/FreeRTOS/FreeRTOSStub.cpp` — core fix: task self-deletion via longjmp, yield/delay checks
- `Devices/MystrixSIL/CMakeLists.txt` — EXPORTED_FUNCTIONS updated with all 15 exports
- `Devices/MystrixSIL/Device.cpp` — added 3 new WASM exports for keypad state
- `Devices/MystrixSIL/web-ui/src/stores/wasm.js` — added runtime state getters
- `Devices/MystrixSIL/web-ui/src/stores/input.js` — added runtime state stores and polling
- `Devices/MystrixSIL/web-ui/src/stores/logs.js` — removed error suppression
- `Devices/MystrixSIL/web-ui/src/components/DevicePanel.svelte` — polls runtime keypad state
- `Devices/MystrixSIL/web-ui/src/components/InputPanel.svelte` — shows runtime state section
