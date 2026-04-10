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

## Round 3 Plan
- Fix the remaining `RuntimeError: table index is out of bounds` crash in the UI callback layer
- Replace all `std::function`-based callbacks in the UI layer with a lightweight `UICallback<Sig>` type using named template function pointers
- Add `-sALLOW_TABLE_GROWTH=1` Emscripten linker flag as safety net

## Round 3 Results

### Root Cause Analysis
After Round 2 fixed the FreeRTOS stub `vTaskDelete` crash, the `table index is out of bounds` error moved downstream to the UI callback layer. The crash occurred after `UI: Register UI Application Launcher` — the Shell installs lambda callbacks (OnPress, SetInputEventHandler, SetColorFunc) via `std::function`.

`std::function` uses complex type-erasure with internal vtable/invoker mechanisms. In WASM, these invokers go through `call_indirect` instructions that check the indirect function table bounds. Emscripten's libc++ `std::function` implementation creates many internal invoker/destroyer functions whose table entries can exceed the fixed-size table.

This is the same root cause pattern as the `Application_Info::factory`/`destructor` crash fixed in Round 2 (commit e6ff9048), where replacing `std::function` with named template function pointers eliminated the crash.

### Fix: UICallback<Sig> (`OS/UI/UICallback.h`)
Created a lightweight type-erased callable that replaces `std::function` throughout the UI layer:
- Stores `void* state_`, `InvokeFn invoke_`, `DestroyFn destroy_`
- Uses named static template member functions `Invoke<Stored>()` and `Destroy<Stored>()` — NOT lambdas — producing stable, predictable WASM indirect call table entries
- Move-only semantics, `explicit operator bool()`, direct `operator()` invocation (no `*` dereference)
- `Reset()` method replaces `unique_ptr::reset()`
- Heap-allocates captured state via `new Stored(...)` / `delete`
- Compatible with all existing lambda call sites — no changes needed in callers

### Fix: -sALLOW_TABLE_GROWTH=1 (`CMakeLists.txt`)
Added `-sALLOW_TABLE_GROWTH=1` to the Emscripten linker flags as a safety net, allowing the WASM indirect call table to grow dynamically if needed.

### Files Changed
- `OS/UI/UICallback.h` — **NEW** lightweight type-erased callable
- `OS/UI/UI.h` — replaced 7 `std::unique_ptr<std::function<...>>` members with `UICallback<...>`, converted setter methods to inline templates
- `OS/UI/UI.cpp` — removed out-of-line setter implementations (now inline), removed `*` dereference on virtual method invocations
- `OS/UI/Component/UIComponent.h` — replaced `enableFunc` member and `SetEnableFunc` setter
- `OS/UI/Component/UIButton.h` — replaced 3 callback members (pressCallback, holdCallback, colorFunc) and their setters/invocations
- `OS/UI/Component/UIToggle.h` — removed `*` dereference on `colorFunc` and `pressCallback`
- `OS/UI/Component/UISelectorBase.h` — replaced 3 callback members, converted `.reset()` to `.Reset()`, removed `*` dereference
- `OS/UI/Component/UISelector.h` — replaced 2 callback members, updated `= nullptr` to `.Reset()`
- `OS/UI/Component/UIItemSelector.h` — replaced template callback member
- `OS/UI/Component/UINumModifier.h` — replaced callback member, removed `<functional>` include
- `OS/UI/Component/UINumSelector.h` — replaced callback member
- `OS/UI/Component/UITimedDisplay.h` — replaced renderFunc member, removed `*` dereference on `enableFunc`
- `OS/UI/Component/UI4pxNumber.h` — replaced 2 callback members
- `OS/UI/Utilities/ColorPicker.cpp` — replaced 2 direct `std::function` members in local classes with `UICallback`
- `Devices/MystrixSIL/CMakeLists.txt` — added `-sALLOW_TABLE_GROWTH=1` linker flag
- `Applications/Note/ArpDirVisualizer.h` — removed `*` dereference on `enableFunc`
- `Applications/Note/ScaleModifier.h` — replaced `std::unique_ptr<std::function<...>>` with `UICallback`
- `Applications/Sequencer/NotePad.cpp` — removed `*` dereference on `enableFunc`
- `Applications/Sequencer/ScaleModifier.h` — replaced `std::function` and `std::unique_ptr<std::function<...>>` with `UICallback`
- `Applications/Sequencer/ScaleModifier.cpp` — updated implementations for UICallback
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/UI/MatrixOS_UI.cpp` — removed unused `<functional>` include
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/UI/Components/MatrixOS_UIButton.cpp` — removed unused `<functional>` include
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/UI/Components/MatrixOS_UISelector.cpp` — removed unused `<functional>` include
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/UI/Components/MatrixOS_UIComponent.cpp` — removed unused `<functional>` include
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/UI/Components/MatrixOS_UI4pxNumber.cpp` — removed unused `<functional>` include

## Round 4: Layout and Tool Model Refactor

### Goal
Refactor the dashboard from a page-switcher layout to a proper three-column architecture matching the updated tool spec.

### Changes Made

#### Layout Architecture
- **App.svelte**: Rewired from 4-page switcher to 3-section global nav + conditional right-side tool area (ToolTray + ToolPanelStack only shown on Device page).
- **LeftNav.svelte**: Reduced to 3 sections (Device, Settings, Firmware). Input/Logs/Runtime removed from left nav.
- New **ToolTray.svelte**: Right-edge icon bar with 8 tool slots. Collapsible with hover labels.
- New **ToolPanelStack.svelte**: Renders open tools as vertically stacked split panels with close buttons.

#### New Store
- **stores/tools.js**: Manages `openTools` array with localStorage persistence. Exports `toggleTool()`, `closeTool()`, and `deviceTools` registry.

#### New Tool Panel Shells
- **tools/UIPanel.svelte** — placeholder for UI stack inspection
- **tools/MIDIPanel.svelte** — placeholder for MIDI monitor/injection
- **tools/HIDPanel.svelte** — placeholder for HID activity
- **tools/SerialPanel.svelte** — placeholder for serial console
- **tools/UsagePanel.svelte** — placeholder for RAM/tasks/resource stats (merged)

#### New Page Placeholders
- **SettingsPage.svelte**, **FirmwarePage.svelte** — placeholder pages

#### Top Bar
- Removed DFU button
- Renamed "Reboot" → "Reset"
- Replaced `v3.3.0` with full build identity: `Matrix OS 3.3 Development • <hash> • Clean/Dirty`

#### Build Identity Pipeline
- **Device.cpp**: `MatrixOS_Wasm_GetVersionString` now returns `MATRIXOS_VERSION_STRING.c_str()`
- **vite.config.js**: Injects `__GIT_HASH__` and `__GIT_DIRTY__` at build time
- **stores/wasm.js**: New `buildIdentity` derived store

### Validation
- `npm run build` succeeds cleanly (4.92s)

### What Remains
- Browser runtime validation (needs WASM binary)
- Tool panel content implementation
- Settings/Firmware page content
- Scenario/recording/MIDI tooling
- Debug API / WebSocket transport
