# Python Improvement Log

## 2026-06-15: MicroPython smoke reliability and example parity

Goal: improve Phase 3 debugging before changing app code or declaring example parity broken.

### Finding

- SameGame manual testing was correct: the app was not the root problem. The earlier automated failure
  came from WebUI smoke timing/state isolation, not from a required SameGame workaround.
- `input.execute` used independent timers for Press/Release and keypad hold ticks. When the browser
  main thread was delayed, the hold tick could run before a due Release action, turning a short press
  into a hold in automated tests.
- `--smoke-dev` reused fixed ports (`5174` / `4012`). On Windows, old dev server or Chrome child
  processes can survive a test run, so the next run could connect to stale runtime state.
- Dice exposed a real robustness issue: bad or stale underglow period values could reach
  `ColorEffects` and trigger a WASM `remainder by zero` instead of a Python-level error.

### Implemented

- WebUI input scheduling now runs due Press/Release actions before `tickKeypad()`, then performs a
  final settle tick.
- `input.get` diagnostics include recent injected input events.
- `bridge.status` reports pending request details, and bridge requests can time out with structured
  JSON-RPC errors instead of leaving opaque pending requests.
- `--smoke-dev` now chooses fresh free ports by default, preventing stale dev runtime contamination.
- Smoke failure diagnostics include bridge status, runtime/app state, Python status/output, active
  input trace, LED frame summary, MatrixOS logs, and emulator host errors.
- Dice clamps persisted underglow effect and period values to the selector-supported range.
- MicroPython `ColorEffects` bindings reject period `0` with `ValueError` rather than crashing the
  WASM runtime.

### Verification

- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-build --skip-web-build`
  passed.
- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite examples --example same_game.py`
  passed.
- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite examples --example dice.py`
  passed.
- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite examples`
  passed for Pixel Art, SameGame, Gomoku, and Dice.
- Full `verify:micropython` passed after rebuilding `MatrixOSHost`, validating wasm, packaging the
  WebUI runtime, and building the production WebUI.

### Remaining Risk

- One post-rebuild full examples run saw the runtime bridge stop responding during the later
  startup/stop sanity phase. `gomoku.py` single-example smoke and a full examples rerun both passed.
  Treat this as lifecycle/stress work for Phase 3, not as evidence that Gomoku or SameGame need
  app-local workaround code.

## 2026-06-15: Lifecycle smoke suite

Goal: turn the runtime responsiveness risk into a focused regression suite.

### Implemented

- Added a `lifecycle` suite to `micropython-smoke.mjs`.
- The suite repeatedly starts and exits short scripts, checking `session.ping` after each run.
- It verifies `ColorEffects.rainbow(0)` raises `ValueError` and that a later script can still run.
- It starts a long-running app, stops it through `python.stop`, then verifies `python.status` and
  `session.ping` still respond.

### Verification

- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite lifecycle`
  passed.
- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev`
  passed with suites `core, filesystem, ui, lifecycle, examples`.

## 2026-06-15: Runtime debug endpoint

Goal: make Phase 3 runtime failures diagnosable without adding Python example workarounds.

### Implemented

- `MicroPythonRuntime` now exposes read-only stats for initialization state, heap size, GC usage, max
  free heap, GC block counters, and C stack usage.
- The Python app exposes a simulator debug JSON snapshot for active loop state, REPL/app state, and
  runtime memory stats.
- MystrixSim exports `MatrixOS_Wasm_GetPythonDebugJson`.
- MystrixSim exports staged Python package metadata through `MatrixOS_Wasm_GetPythonStagedFilesJson`.
- WebUI RPC adds `python.debug`, and `python.status` includes the same payload under `debug`.
  The payload now includes active `scriptPath` plus staged file path/size/entry metadata without
  embedding script contents.
- Smoke failure diagnostics now include `python.debug`.
- Lifecycle smoke verifies that a running app reports initialized runtime memory stats, active script
  metadata, staged entry metadata, and that `python.debug` returns inactive after `python.stop`.

### Verification

- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-build --skip-web-build`
  passed.
- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-web-build`
  passed, including MatrixOSHost rebuild, wasm validation, and runtime packaging.
- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite lifecycle`
  passed.

## 2026-06-13: UI input API shape

Goal: make Python UI input feel Pythonic without reintroducing native-to-Python input callback
instability.

### Findings

- PikaPython packages commonly split APIs into a native underscore module plus a Python facade module.
  Examples in `C:\Users\NengzhuoCai\Downloads\PikaPython-master` include `package/modbus/modbus.py`,
  `package/network/network.py`, and `package/requests/requests.py`.
- `package/pika_lvgl` is the closest reference for a UI-heavy binding. It keeps native C wrappers
  narrow, stores native pointers/objects behind Pika objects, and exposes a Python-facing object model
  instead of forcing application code to touch C data directly.
- Pika core and `pika_lvgl` both commonly return native objects by constructing `newNormalObj(...)`
  and returning `arg_newObj(...)`. That pattern is not inherently wrong.
- The risky shape is returning fresh native MatrixOS event objects from a UI input callback path. The
  UI loop is already executing Python callbacks; mixing that with nested native object creation and
  object lifetime ownership made the simulator hang in prior tests. Keeping UI polling as a primitive
  code path avoids that re-entrancy/lifetime edge.
- Native modules should expose small, stable primitives. Python facade modules can wrap those into
  nicer classes and methods.
- Declaring a pure Python facade class in a `.pyi` file makes the generator expect native C functions
  for that class. Keep facade-only classes in `.py`, not in the native `.pyi`.
- Returning a native `InputEvent` object directly from `UI.PullInput()` compiled, but running the
  MystrixSim runtime could hang during Python app launch or shortly after. A previous smoke also showed
  an incorrectly initialized object returning `0 0 0` for cluster/member/state when constructed through
  the package-object path.

### Current Design

- Native `_MatrixOS_UI.UI.PullInputCode()` returns an integer code, or `-1` when the queue is empty.
- Public Python `MatrixOS_UI.UI.pull_input()` wraps that code as a pure Python `KeyEvent`, or returns
  `None`.
- `KeyEvent` exposes Pythonic helpers such as `cluster_id()`, `member_id()`, and `key_state()`.
- UI instances install input capture automatically. Apps do not call `SetInputHandler()`.
- The native input capture handler returns `false` so normal UI components such as `UIButton.OnPress()`
  still receive input.
- Unread queued input is cleared after each UI loop callback.
- Lowercase facade methods are added where they improve application code:
  `ui.pull_input()`, `event.cluster_id()`, `event.member_id()`, `event.key_state()`,
  `event.is_pressed()`, `event.is_released()`, and similar helpers.

### Why this is more Pythonic

Application code sees:

```python
event = ui.pull_input()
if event is not None:
    if event.key_state() == KeyState.PRESSED:
        handle(event.cluster_id(), event.member_id())
```

The app no longer decodes bitfields, checks a sentinel integer, or enables a separate input handler.

### Follow-up

- Add optional `KeyEvent` helpers such as `IsPressed()`, `IsReleased()`, `IsFunctionKey()`, `X()`, and
  `Y(width=8)` if examples keep repeating those checks.
- Add a simulator regression that starts a Python UI, injects a grid press, and verifies `pull_input()`
  returns a non-`None` object with the expected cluster/member/state.
- Revisit native object returns after adding a focused Pika object lifetime test. Avoid using native
  `InputEvent` returns in hot UI paths until that test is stable.

## 2026-06-13: Pythonic facade pass

Goal: make the public Python API pleasant without destabilizing the generated native bindings.

### Decisions

- Keep PascalCase only in private `_MatrixOS_*` native bindings.
- Public Python facade modules expose Pythonic endpoint names only.
- Avoid `@property` for now. PikaPython support is less proven than normal methods, and methods such
  as `point.x()` or `color.rgb()` are still much nicer than C++ getter names.
- Avoid pure-Python facade classes in native `.pyi` files. The generator treats `.pyi` classes as
  native binding declarations.
- Avoid self-referential return annotations such as `-> Color` inside facade classes. CPython import
  evaluates those annotations and can fail before tests run; they add little value inside PikaPython.

### Implemented

- `MatrixOS_SYS`: added `sleep_ms`, `delay_ms`, `millis`, `micros`, `launch_app`,
  `launch_app_by_id`, `open_settings`, `reboot`, `bootloader`, and `get_version`.
- `MatrixOS_LED`: added `set_color`, `set_color_xy`, `set_color_by_id`, `set_pixel`, `fill`,
  `clear`, `fill_partition`, `update`, `show`, `count`, and layer helper aliases.
- `MatrixOS_Color`: added component helpers `r`, `g`, `b`, `w`, `rgb`, `wrgb`, setters, and module
  constructors `rgb`, `rgbw`, `hex`.
- `MatrixOS_Point`: added `x`, `y`, `set_x`, `set_y`, `rotate`, and module constructor `point`.
- `MatrixOS_UI`: added lowercase `pull_input()` and richer pure-Python `KeyEvent` helpers. UI
  components are available through `MatrixOS.UI.Button`, `MatrixOS.UI.Selector`, and
  `MatrixOS.UI.Number`.
- `MatrixOS`: the top-level import now exposes Pythonic namespace facades. `MatrixOS.Input`,
  `MatrixOS.SYS`, `MatrixOS.LED`, `MatrixOS.MIDI`, `MatrixOS.NVS`, `MatrixOS.HID`, `MatrixOS.USB`,
  and `MatrixOS.UIUtility` only expose lowercase endpoint methods; generated/native PascalCase names
  remain private implementation detail modules.
- `MatrixOS_Input`: lowercase `get_event()` returns a Python `InputEventView`, and `get_position()`
  returns a Pythonic point facade with `x()` and `y()`.
- `MatrixOS_InputEvent`, `MatrixOS_InputId`, and `MatrixOS_KeypadInfo`: native class wrappers keep their
  generated class names and gain lowercase helpers. The Python-only proxy classes are named
  `InputEventView`, `InputIdView`, and `KeypadInfoView` so they do not pretend to be generated native
  classes.
- `InputEvent` and `InputId` use `member_id()` as the single name for the cluster-local input member.
  They also expose `cluster_name()` for readable input routing.
- `MatrixOS.Input.try_get_point(input_id)` returns a point facade or `None` when an input cannot be
  represented as a coordinate.
- Keypad payloads expose both `pressure()` and `velocity()`; event helpers expose `key_pressure()` and
  `key_velocity()`.
- `InputEvent.info()` is an alias for `keypad()`, giving Python code a short neutral name for the
  event payload while keeping the MatrixOS-specific `Keypad()` compatibility method.
- `MatrixOS_UI.KeyEvent.x()` and `y()` decode the primary grid member id as `member % 8` and
  `member // 8`. Non-grid events still use packed low-byte/high-byte fallback decoding.
- Proxy objects expose `raw()` for code that needs to pass through to lower-level implementation APIs.
  App code should use the lowercase endpoints from `import MatrixOS`.
- `MatrixOS_NVS`, `MatrixOS_MIDI`, `MatrixOS_USB`, and `MatrixOS_Utils`: added small lowercase helpers
  and length-hiding helpers such as `MIDI.send_sysex(port, data)`.
- UI facades now have lowercase component helpers: `ui.start()`, `ui.add(...)`, `ui.set_loop_func(...)`,
  `button.on_press(...)`, `button.set_enable_func(...)`, and matching helpers for selector and 4px
  number components.

### Example Migration

- `Applications/Python/examples/pixel_art.py` now uses the lowercase input, LED, point, and system
  helpers.
- `Applications/Python/examples/same_game.py` now uses `MatrixOS.UI.Button`, `MatrixOS.Timer`,
  `MatrixOS.ColorEffects`, `MatrixOS.Input.try_get_point()`, and lowercase event helpers.

### Tests

- `Applications/Python/tests/test_ui_key_event.py` covers UI `KeyEvent` wrapping and decoding.
- `Applications/Python/tests/test_pythonic_facades.py` covers `Color`, `Point`, and `LED` facades with
  CPython native stubs.
- `Applications/Python/tests/test_input_facades.py` covers input proxy wrapping and native ID unwrapping.
- `Applications/Python/tests/test_ui_component_facades.py` covers UI button lowercase aliases.

### Runtime Verification

- Static tests, Pika bytecode generation, and MystrixSim host build pass.
- After opening a live MystrixSim page through Chrome, the WebSocket RPC bridge responded to
  `python.status` and `runtime.getState`.
- A Python app smoke using the new lowercase facades completed successfully:
  `SYS.millis()`, `Color.rgb()`, `Point.point()`, `LED.clear()`, `LED.set_color_xy()`,
  `LED.show()`, `Input.get_event()`, and `Input.function_key()` all ran without Pika exceptions.
- A blocking Python app confirmed that WebSocket RPC calls are still processed while Python app mode is
  active: an `input.execute` call returned in about 89 ms during a 5 second `SYS.sleep_ms()` script.
  This points away from facade syntax as the source of the earlier UI input issue.
- WebUI RPC `input.execute` now injects through `MatrixOS_Wasm_KeyInfoEvent`, matching the hardware
  mirror path. The old RPC helper compressed input into a boolean press/release call, so Python input
  smoke tests were not exercising the same event bridge as physical device forwarding.
- Live MystrixSim RPC smoke now starts a Python UI, injects `grid:2,3` press/release, and receives
  `event 1 26 2` then `event 1 26 5` from `ui.pull_input()`. This verifies the pure-Python
  `KeyEvent` wrapper, the native `PullInputCode()` bridge, and the simulator input injection path.
- Live raw input polling through `MatrixOS.Input.get_event()` is not a good app-level pattern yet.
  The OS queue is global, UI startup clears it, and UI loops consume it. The stable path for Python UI
  apps is `ui.pull_input()`, which copies keypad events from the UI input handler into a Python-owned
  queue before normal UI processing continues.
- `Input.get_event()` remains available as a low-level compatibility wrapper, but new examples should
  prefer UI-managed input unless a future Python app runner owns a dedicated input loop.
- A browser page may keep an older WASM module after `session.reset`; when validating newly generated
  Pika assets, use a fresh runtime bridge or full page reload before treating RPC smoke output as final
  evidence.
- Current conclusion: the original UI native-object-return hang was a high-risk Pika object
  lifetime/re-entrancy shape. Keeping the hot path primitive-first and wrapping it in Python is more
  stable and closer to the pattern used by `pika_lvgl`.

### Next Questions

- If PikaPython property support proves stable later, `color.r` and `point.x` properties may be nicer
  than method calls. For now, method calls are the safer API.
- Consider adding a dedicated Python app input service if non-UI Python scripts need reliable event
  polling. It should mirror the UI-owned queue approach instead of exposing the global OS queue as the
  primary high-level API.

## 2026-06-13: Partition and Cluster Introspection

Goal: expose device layout information to Python apps without making app code depend on C++ binding
objects directly.

### Implemented

- `_MatrixOS_LED` now exposes native LED partition primitives:
  `GetPartitionCount()`, `GetPartitionName(index)`, `GetPartitionStart(index)`,
  `GetPartitionSize(index)`, `GetPartitionType(index)`, `GetPartitionDefaultMultiplier(index)`, and
  `GetPartitionIndex(name)`.
- `MatrixOS_LED` originally wrapped those primitives with `LEDPartition` objects and split helpers;
  the MicroPython public API later converged on `partitions()` and
  `get_partition(index_or_name)` without exposing `partition_count()` or `get_partition_by_name()`.
- `InputId(cluster_id, member_id)` is now constructible from Python, so UI `KeyEvent.id()` can return
  a real MatrixOS input id instead of forcing apps to carry separate cluster/member integers.
- `MatrixOS_Input` now exposes keypad cluster helpers:
  `get_cluster(target)`, `keypad_clusters()`, `keypad_cluster(name="")`, and `primary_grid()`.
- Native input cluster objects are wrapped with `InputClusterView` on Pythonic APIs. Raw native
  `_MatrixOS_*` calls remain implementation details, while app-facing helpers return objects with
  lowercase methods.
- `MatrixOS_Input.get_state()` now returns `InputSnapshotView`, matching the existing event and keypad
  proxy style.

### API Shape

Typical app code can now ask the device what exists before assuming an 8x8 grid:

```python
grid = MatrixOS.Input.primary_grid()
if grid is not None:
    width = grid.width()
    height = grid.height()

for partition in MatrixOS.LED.partitions():
    print(partition.name, partition.start, partition.size)
```

The design keeps native modules as small data providers and keeps policy, naming, and Python
ergonomics in the facade layer. This matches the `pika_lvgl` pattern more closely than expanding the
native bindings into a second full object model.

### Tests

- `test_pythonic_facades.py` covers LED partition lookup and partition metadata helpers.
- `test_framework_facades.py` covers dimension, input cluster, and input snapshot views.
- `test_input_facades.py` covers input proxy unwrapping, cluster lookup, keypad cluster selection, and
  primary grid access.
- `test_ui_key_event.py` covers `KeyEvent.id()` and the pure-Python event-to-`InputId` conversion.
- MystrixSim RPC smoke verified LED partition discovery and input cluster discovery in the live
  PikaPython runtime. The stable remote path is currently `python.stage` followed by
  `python.runStaged`; `python.runText` can race against short scripts that exit before the RPC wait
  loop observes app mode.
- Runtime smoke also found that dense expressions containing multiple object method calls can
  misparse in PikaPython. Examples should assign object method results to local variables before
  combining them in prints or larger expressions.
- MIDI packet factory results are native packet objects, so lowercase methods attached to the Python
  subclass are not reliable on those returned values. `MatrixOS_MIDI` now re-exports module-level
  field helpers such as `velocity(packet)`, `status(packet)`, and `set_velocity(packet, value)` for a
  Pythonic path that works with native-returned packets.
