# MicroPython Runtime Migration TODO

> Historical note: this file is an archive of the original PikaPython to
> MicroPython migration plan. Unchecked boxes below do not represent current
> open implementation work. Current status and deferred polish items live in
> `docs/micropython-runtime-phase3.md`.

This document tracks the ongoing migration from the current PikaPython runtime to MicroPython.

Production-ready follow-up work was previously tracked in `docs/micropython-runtime-phase2.md`. This original TODO records migration history and first-cut status only.

## Goal

Replace the MatrixOS Python runtime with MicroPython, redesign the public Python API where needed to match MicroPython conventions, update MystrixSim/WebUI integration, and keep the four shipped Python examples running on the new runtime.

## Acceptance Criteria

- [x] `Applications/Python` builds against MicroPython instead of PikaPython.
- [x] The Python app can launch in script mode and REPL mode.
- [x] MystrixSim/WebUI Python panel can enter REPL, stage a script, run it, send input, and stream output.
- [ ] The MatrixOS Python API is MicroPython-friendly, complete enough for native app parity, and documented by tests or examples.
- [ ] The four examples run under MicroPython and faithfully match the original app behavior:
  - [ ] `pixel_art.py`
  - [ ] `same_game.py`
  - [ ] `gomoku.py`
  - [ ] `dice.py`
- [x] The old Pika-specific generated binding path is removed or isolated so it is not part of the active runtime.
- [x] MystrixSim `MatrixOSHost` builds and runtime wasm validates.
- [x] Native/device build path has a clear MicroPython port story, even if target-specific tuning remains.

## Phase 1: Audit And Design

- [x] Identify current PikaPython runtime entry points:
  - `Applications/Python/Python.cpp`
  - `Applications/Python/Python.h`
  - `Applications/Python/CMakeLists.txt`
- [x] Identify current WebUI integration points:
  - `Devices/MystrixSim/HostIO.cpp`
  - `Devices/MystrixSim/WebUI/src/handles/python.js`
  - `Devices/MystrixSim/WebUI/src/stores/rpc.js`
  - `Devices/MystrixSim/WebUI/src/components/tools/PythonPanel.svelte`
- [x] Define MicroPython lifecycle:
  - [x] Initialize heap and runtime.
  - [x] Register MatrixOS modules.
  - [x] Execute staged/file script.
  - [x] Detect and call optional `loop()`.
  - [x] Deinitialize runtime on app exit.
- [x] Define MicroPython REPL behavior:
  - [x] Minimal line-buffer REPL.
  - [x] Raw input/output hooks mapped to existing MatrixOS Python I/O bridge.
  - [x] Friendly multiline/paste behavior.
- [x] Decide filesystem/import strategy:
  - [x] MystrixSim staged scripts.
  - [x] Device storage scripts.
  - [x] External import and `open()` use lightweight MatrixOS MicroPython port hooks rather than a full VFS pass.

## Phase 2: Dependency And Build

- [x] Add MicroPython source dependency:
  - Preferred: `Library/micropython` as a git submodule pinned to a release tag or commit.
  - Candidate release: `v1.28.0`.
  - Current pin: `v1.28.0`.
- [x] Add a MatrixOS MicroPython port directory:
  - Proposed path: `Applications/Python/MicroPythonPort`.
- [x] Add `mpconfigport.h`, `mphalport.h`, and required port glue.
- [x] Update `Applications/Python/CMakeLists.txt` to compile the MicroPython core and MatrixOS port.
- [x] Keep the first build minimal:
  - [x] No filesystem import at first if it blocks progress.
  - [x] No full UI binding at first if LED/Input/SYS can prove runtime viability.
- [x] Build `MatrixOSHost` with MicroPython linked.

## Phase 3: MatrixOS MicroPython API

Design the public API for normal MicroPython usage. Avoid preserving PikaPython-specific workaround shapes.

- [x] `import matrixos` or `import MatrixOS` naming decision.
  - First MicroPython cut keeps `import MatrixOS` to match the existing MatrixOS app scripts while removing Pika-specific PascalCase method shapes.
- [x] `Color` strategy:
  - [x] Accept RGB integers and `(r, g, b[, w])` tuples for LED APIs.
  - [x] Return packed RGB integers from color helper/effect APIs for MicroPython-friendly simplicity.
- [x] `Point` strategy:
  - [x] Use plain `(x, y)` tuples for positions.
- [x] `SYS`:
  - [x] `millis()`
  - [x] `micros()`
  - [x] `sleep_ms()`
  - [x] `yield_()` / `task_yield()`
  - [x] `exit_app()`
- [x] `LED`:
  - [x] `clear()`
  - [x] `fill(color)`
  - [x] `set_color_xy(x, y, color)`
  - [x] `set_index(index, color)`
  - [x] `fill_partition(name, color, layer=...)`
  - [x] `update()`
  - [x] partition discovery API.
- [x] `Input`:
  - [x] `get_event(timeout_ms=0)`
  - [x] `function_key()`
  - [x] `try_get_point(input_id)`
  - [x] keypad event IDs and point lookup.
  - [x] event state, velocity, pressure, release, and aftertouch fields.
  - [x] API introspection smoke coverage.
- [ ] `UI`:
  - [x] Add native wrapper module surface:
    - `MatrixOS.UI.UI`
    - `MatrixOS.UI.Button`
    - `MatrixOS.UI.Selector`
    - `MatrixOS.UI.Number`
    - `MatrixOS.UI.Toggle`
  - [x] Route wrapper behavior through native C++ `UI` / `UIComponent` so LED layers, fade, function-key release exit, input consume, and TextScroll behavior match native apps.
  - [x] Add smoke coverage for constructing, configuring, adding, clearing, and closing UI components.
  - [ ] Add interactive RPC tests for UI behavior:
    - [x] Button release callback.
    - [x] Button hold callback.
    - Button hold TextScroll.
    - [x] Selector release callback.
    - [x] Function-key release exits child UI without leaking to parent.
    - [x] Function-key hold exits app where the app implements that policy.
  - [x] Add wrappers for UI utilities:
    - `text_scroll`
    - `color_picker`
    - `number_selector`
- [x] `ColorEffects`:
  - [x] `rainbow()`
  - [x] `color_breath()`
  - [x] `color_strobe()`
  - [x] `color_saw()`
- [x] `NVS`:
  - [x] primitive typed get/set API.
- [x] `MIDI`, `USB`, `HID`, `FileSystem`, `Logging`:
  - [x] Implement public MicroPython wrappers for MIDI, USB, HID, FileSystem, and Logging.

## Phase 4: Runtime Integration

- [x] Replace Pika runtime calls in `Python.cpp`:
  - [x] `pikaPythonInit()`
  - [x] `pikaVM_run()`
  - [x] `pikaVM_runFile()`
  - [x] `pikaPythonShell()`
  - [x] `obj_run(..., "loop()")`
- [x] Add MicroPython script execution helpers:
  - [x] run string
  - [x] run file
  - [x] call `loop()`
  - [x] report exception text to `matrixos_python_write_bytes`
  - [x] preserve traceback filename/line information in WebUI RPC output.
  - [x] keep UI callback exceptions contained so the native UI loop stays responsive.
- [x] Preserve existing Python session modes:
  - [x] none
  - [x] repl
  - [x] app
- [x] Preserve staged script behavior in MystrixSim.
- [x] Preserve app launch through Shell:
  - [x] `MatrixOS::SYS::ExecuteAPP("203 Systems", "Python", args)`

## Phase 5: WebUI And Remote Debugging

- [x] Confirm these RPC calls still work:
  - [x] `python.status`
  - [x] `python.enterRepl`
  - [x] `python.stage`
  - [x] `python.runStaged`
  - [x] `python.runText`
  - [x] `python.input`
  - [x] `python.getOutput`
  - [x] `python.subscribe`
- [x] Update UI labels if needed:
  - [x] "Python" stays generic.
  - [x] Runtime detail can say MicroPython.
- [x] Verify terminal input behavior:
  - [x] Enter
  - [x] Backspace
  - [x] Ctrl-C
  - [x] Ctrl-D
  - [x] Multiline block paste.
- [x] Add/adjust WebUI smoke testing path for MicroPython.

## Phase 6: Examples

Examples should be rewritten to normal MicroPython-friendly Python, not Pika workaround style. They should use native UI wrappers when the original app uses native UI instead of open-coded LED/Input overlays.

- [ ] `pixel_art.py`
  - [x] Remove Pika-specific patterns.
  - [x] Verify LED writes and app launch.
  - [x] Add RPC smoke coverage for picker color row, function-key picker toggle, and painting selected color.
  - [ ] Finish matching original persistence and side/off-grid cluster behavior.
- [ ] `same_game.py`
  - [x] Use normal closures/classes where appropriate if MicroPython supports them well.
  - [x] Verify app launch and input path through RPC.
  - [x] Add RPC smoke coverage for board fill, adjacent group removal, settings color-count update, and NVS persistence.
  - [ ] Finish matching original scoring TextScroll, animations, and native settings UI behavior.
- [ ] `gomoku.py`
  - [x] Verify app launch and input path through RPC.
  - [x] Add RPC smoke coverage for release-to-place, settings winning-length update, first-player update, and NVS persistence.
  - [ ] Finish matching original win/reset animations and native settings UI behavior.
- [ ] `dice.py`
  - [x] Restore normal booleans and conditional expressions.
  - [x] Verify app launch.
  - [x] Verify NVS typed u32 conversion and persistence path.
  - [x] Replace the hand-drawn settings state with native `MatrixOS.UI` components and UI utility wrappers.
  - [x] Add RPC smoke coverage for dice face render, roll input, settings number-mode update, and NVS persistence.
  - [ ] Finish matching original visual effects and deeper settings submenus.

## Phase 7: Verification

- [x] Host/unit tests for Python API wrappers.
  - No CPython facade wrapper is part of the MicroPython cut.
  - Current API coverage is `api_introspection.py` through the WebUI RPC smoke test because the native `MatrixOS` module is not CPython-importable.
- [x] `python -m py_compile` for all examples, as a CPython syntax sanity check.
- [x] `cmake --build build\MystrixSim --target MatrixOSHost --parallel`.
- [x] `node Devices\MystrixSim\tools\validate-runtime-wasm.mjs build\MystrixSim\Devices\MystrixSim\MatrixOSHost.wasm`.
- [x] WebUI manual/smoke test:
  - [x] REPL starts and prints banner.
  - [x] `print("hello")` works.
  - [x] `python.runText` / `python.stage` + `python.runStaged` can run scripts through the WebUI RPC bridge.
  - [x] Each of the four examples can launch.
- [x] Reusable WebUI RPC smoke test:
  - [x] `npm run smoke:micropython -- --ws ws://localhost:4012`
  - [x] Includes API introspection assertions.
  - [x] Includes native UI wrapper construction/configuration assertions.
  - [x] Includes UI interaction assertions for button release, button hold, selector release, function-key release consumption, and function-key hold app-exit policy.
  - [x] Includes Pixel Art interaction assertions for picker rendering, function-key release toggle, and grid paint.
  - [x] Includes SameGame interaction assertions for group removal, settings color-count update, and NVS persistence.
  - [x] Includes Gomoku interaction assertions for release-to-place, settings update, and NVS persistence.
  - [x] Includes Dice interaction assertions for roll/settings mode update and NVS persistence.
- [ ] Reusable WebUI RPC interaction tests for original app parity.
  - [x] UI callback exception smoke verifies traceback output and UI responsiveness.
- [x] WebUI production build:
  - [x] `npm run build` in `Devices/MystrixSim/WebUI`.
- [x] Document known limitations before merging.

## Open Questions

- [x] Should the public module be `MatrixOS`, `matrixos`, or both?
- [x] Should UI wrappers be in the first MicroPython cut, or should examples use raw input until native UI binding is stable?
- [x] Should Python helper modules be frozen into firmware or loaded from filesystem/staged text?
- [x] How much of the old Pika facade test suite should be ported versus replaced?
- [x] What minimum device memory budget is acceptable for MicroPython?

## Progress Log

- 2026-06-14: Created migration TODO and identified current Pika runtime/WebUI entry points.
- 2026-06-14: Added MicroPython as `Library/micropython` submodule at `v1.28.0`.
- 2026-06-14: Reviewed MicroPython `ports/embed`; it is the preferred integration path because it generates a self-contained source package for embedding into an existing C/C++ project.
- 2026-06-14: Identified that `Applications/Python/CMakeLists.txt` currently GLOBs all runtime sources, so Pika must be isolated before adding MicroPython port sources.
- 2026-06-14: Added `Applications/Python/MicroPythonPort`, generated `Applications/Python/MicroPythonEmbed`, and replaced active Pika runtime calls in `Python.cpp`.
- 2026-06-14: Added first native MicroPython `MatrixOS` module with minimal `SYS`, `LED`, and `Input` APIs.
- 2026-06-14: Verified `MatrixOSHost` builds with MicroPython linked when `EM_CACHE` points at the workspace cache, then validated the wasm output.
- 2026-06-14: Added a minimal line-buffer MicroPython REPL and verified WebUI production build.
- 2026-06-14: Migrated the four Python examples to the current minimal MicroPython `MatrixOS.SYS`, `MatrixOS.LED`, and `MatrixOS.Input` API and verified CPython syntax with `py_compile`.
- 2026-06-14: Fixed MicroPython tick values to stay within MicroPython small-int range, made no-loop scripts exit automatically, added a runtime-level cooperative delay after each Python `loop()`, and verified all four examples through MystrixSim WebUI RPC.
- 2026-06-14: Verified WebUI RPC script/REPL paths, terminal Enter/Backspace/Ctrl-C/Ctrl-D handling, Python input events via `input.execute`, LED partition discovery, ColorEffects, Utils string hash, and NVS u32 conversion.
- 2026-06-14: Added WebUI RPC `python.stop`, cleaned Python output per launched session, forwarded external WebSocket `*.subscribe` notifications, and re-verified `python.subscribe`, script output, REPL output, input events, and all four examples through MystrixSim RPC.
- 2026-06-14: Added `Devices/MystrixSim/WebUI/tools/micropython-smoke.mjs` and `npm run smoke:micropython`; verified the reusable smoke test covers `runText`, REPL, `python.subscribe`, input events, and all four examples over external WebSocket RPC.
- 2026-06-14: Replaced the stale Pika-era `api_introspection.py` with MicroPython API assertions for `Color`, `Timer`, `SYS`, `LED`, `Input`, `ColorEffects`, `NVS`, and `Utils`; added it to the reusable WebUI RPC smoke test.
- 2026-06-14: Changed no-loop Python scripts to exit on the next app loop instead of during `Setup()`, avoiding app teardown while RPC launch is still returning.
- 2026-06-14: Added multiline REPL buffering with `>>>` / `...` prompts, block-mode blank-line execution, and WebUI RPC smoke coverage for pasted multiline input.
- 2026-06-14: Added `docs/micropython-runtime-port.md` to document native/device port shape, first-cut script loading, why external import/`open()` were initially deferred, and the future VFS work required.
- 2026-06-14: Added lightweight MicroPython file/import port hooks and MystrixSim multi-file staging, so `open()`, external import, and staged helper modules are now covered by smoke tests without requiring full VFS.
- 2026-06-14: Documented first-cut API boundaries; native UI, MIDI, USB, and HID wrappers were later implemented and covered by smoke tests.
- 2026-06-14: Resolved remaining first-cut design questions: keep `MatrixOS` as the public module name, replace old Pika facade tests with RPC smoke for now, and document the current 96 KB MicroPython heap as a target-specific tuning point.
- 2026-06-14: Closed the old host-wrapper unit-test item as replaced by runtime-level `api_introspection.py` coverage in `npm run smoke:micropython`.
- 2026-06-14: Reopened API/app parity scope after reviewing original app behavior. Added native MicroPython wrappers for `MatrixOS.UI.UI`, `Button`, `Selector`, `Number`, and `Toggle`, then verified construction/configuration through WebUI RPC smoke.
- 2026-06-14: Split the MicroPython MatrixOS usermod into subsystem binding files, regenerated the embed package, rebuilt `MatrixOSHost`, repackaged `MatrixOS.msfw`, and re-ran WebUI RPC smoke. `api_introspection.py` now covers SYS version/yield, LED brightness multiplier/layers/fade/pause, Input clusters/capabilities/state lookup, and NVS string/bytes/size/delete helpers.
- 2026-06-14: Added `verify-micropython.mjs` and `npm run verify:micropython` / `verify:micropython:smoke`. The smoke-dev path now starts the WebUI dev server, opens Chrome with a temporary profile, and runs the full WebUI RPC smoke suite automatically.
- 2026-06-14: Parameterized `micropython-smoke.mjs` with `--suite core|filesystem|ui|examples|all` and wired `verify-micropython.mjs --suite` through to the smoke runner, so API, filesystem, UI, and example checks can be isolated.

## API Gap Matrix

| Surface | Current MicroPython state | Needed before calling API complete |
| --- | --- | --- |
| `SYS` | Timing, yield/task_yield, exit, reboot, bootloader, open setting, execute app, version, version_id, and error handler wrappers are implemented. | Decide which dangerous system actions need simulator-only guards or confirmation in tests. |
| `LED` | Fill/set/update/count, partition lookup, brightness, brightness multiplier as integer milli-ratio, layer create/copy/destroy/fade, and pause_update are implemented and covered by API smoke. | Add batch helpers only if examples or parity tests prove they are needed; verify UI fade behavior interactively. |
| `Input` | Event polling, function key, point lookup, cluster discovery, primary grid cluster, state snapshot, inputs-at-position, input-at-position, keypad capabilities, pressure, velocity, and aftertouch fields are implemented. | Add stronger interaction tests for FN short/long press, release consumption, aftertouch, and release velocity. |
| `UI` | Native `UI`, `Button`, `Selector`, `Number`, `Toggle`, and UIUtility wrappers exist and pass smoke construction and interaction tests. SameGame, Gomoku, and Dice setting paths use native UI wrapper surfaces in the current examples. | Add deeper parity tests for TextScroll hold behavior, complex setting submenus, and animation/layer transitions. |
| `NVS` | Typed `u8/u16/u32`, size, delete, bytes, string helpers, and string-key hashing are implemented and covered by API smoke. | Finalize naming and error behavior in `Applications/Python/micropython-api.md`. |
| `MIDI` | `MatrixOS.MIDI` wrapper is implemented: MidiPacket object, packet factories, `is_note_on()` / `is_note_off()`, receive/send, send_sysex, status constants, and port constants. API smoke covers construction/send/get no-data path; WebUI RPC smoke covers MIDI RX injection. | Design message-level SysEx receive helper later if needed. |
| `USB` | `USB.connected()` and CDC connected/available/poll/print/println/flush/read/read_bytes/read_string wrappers are implemented. API smoke covers no-device path; WebUI RPC smoke covers CDC RX injection. | Decide whether USB mode/connect/disconnect management should be exposed to Python. |
| `HID` | `HID.init/reset/ready`, Keyboard tap/press/release/release_all, Gamepad tap/press/release/release_all/button/buttons/axis/dpad, and RawHID get/send wrappers are implemented. API smoke covers no-device path; WebUI RPC smoke covers RawHID RX injection. | Add keycode/gamepad constants if examples need them; decide whether unsupported HID submodules should be documented explicitly. |
| `FileSystem` | `MatrixOS.FileSystem` wrapper is implemented and covered by API smoke: available, exists, mkdir, remove/rmdir, rename, list_dir, read/write bytes, and read/write text. Standard `open()`, external import, and multi-file staged apps are implemented through lightweight port hooks and covered by WebUI smoke. | Full MicroPython VFS object model is still not implemented; current support targets script/app file loading and common text file read/write. |
| `Logging` | `error/warning/info/debug/verbose(tag, message)` wrappers are implemented and covered by API smoke. Script exceptions and UI callback exceptions preserve traceback output in WebUI RPC smoke. | Decide how Python `print` should relate to structured logging. |

## Current Limitations

- MicroPython external filesystem import and Python `open()` are enabled through MatrixOS port hooks. WebUI staging supports multi-file apps under `host:/python/`.
- MIDI, USB, HID, FileSystem, and exception reporting wrappers exist; remaining runtime work is now focused on full app parity, UI edge cases, and the final `print`/structured logging policy.
- The current examples validate runtime, LED, input, timing, NVS, UI wrapper construction, and RPC integration, but do not yet mirror every native C++ app behavior or setting screen.
