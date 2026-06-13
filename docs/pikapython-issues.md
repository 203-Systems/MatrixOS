# PikaPython Issues

This file tracks MatrixOS PikaPython issues found while porting and testing Python apps.

## 2026-06-12: Chained module/class calls are unreliable

Observed while porting `MatrixOSPixelArtApp`.

`MatrixOS.py` exposes `UI` as a class, but some examples and generated stubs suggest `MatrixOS.UI.UI(...)`. PikaPython reported:

```text
Error: method 'MatrixOS.UI.UI' no found.
```

Use one of these forms instead:

```python
import MatrixOS
ui = MatrixOS.UI("Demo", Color(0xFFFFFF))
```

or:

```python
from MatrixOS_UI import UI
ui = UI("Demo", Color(0xFFFFFF))
```

Fix direction:

- Use direct wrapper modules for UI classes for now, for example `MatrixOS_UI.UI(...)` and
  `MatrixOS_UIButton.UIButton()`.
- Do not add a public `MatrixOS.UI` facade until the generated module asset path is better understood.
- Update docs/examples that imply `MatrixOS.UI.UI(...)` or `MatrixOS.UIButton(...)`.

## 2026-06-12: Long Python loops block MystrixSim WebUI debugging

PikaPython app scripts that run an infinite loop with `DelayMs()` can keep the WASM/browser main thread busy enough that Playwright and the JSON-RPC bridge cannot reliably inspect runtime state.

This is mostly a simulator/debugging issue. On device, Python runs in the app task model, but MystrixSim still shares browser execution constraints.

Workaround:

- Use Remote Python RPC for staging and output inspection.
- Avoid repeatedly launching a new Python app while one is already active.
- Prefer short debug scripts or MatrixOS UI callbacks when testing in WebUI.

Fix direction:

- Remote Python RPC now refuses `runText`, `runStaged`, and `enterRepl` while Python is already active.
- Add explicit stop/reset controls if we need tighter remote debug loops.

## 2026-06-12: Python output was hard to inspect remotely

Python app failures often show up only in the Python output stream, not in MatrixOS logs.

Fix direction:

- Add `python.getOutput` and `python.subscribe` to MystrixSim JSON-RPC.
- Keep Python output capture available even when the Python panel is not open.

## 2026-06-12: Negative-step `range()` did not behave reliably

Observed while porting `Matrix-OS-SameGame`.

This setup code did not move pieces down correctly in PikaPython, even though it is valid CPython:

```python
for y in range(HEIGHT - 1, 0, -1):
    ...
```

The game only generated the first row and then stopped filling the board. Rewriting the loop as an explicit `while` loop fixed the runtime behavior:

```python
y = HEIGHT - 1
while y >= 1:
    ...
    y -= 1
```

Fix direction:

- Avoid negative-step `range()` in shipped PikaPython examples for now.
- Add a tiny runtime regression script before relying on full CPython `range()` semantics.

## 2026-06-12: UI wrapper lifecycle and callbacks needed fixes

Observed while trying to port SameGame settings/reset menus to `MatrixOS.UI` and `UIButton`.

A minimal script could construct `MatrixOS.UI`, add a `UIButton`, and call `Start()`, but the Python side did not have a clean way to exit a running UI from a custom callback. While reviewing the binding, `SaveCallbackObjToPikaObj()` also returned `False` on success and `True` on failure, which made callback setter results misleading. It also pre-freed an existing callback before calling `obj_setArg()`, even though `obj_setArg()` already replaces stored args.

Fix:

- Added `UI.Exit()` to the Python UI binding.
- Regenerated the Pika binding from `_MatrixOS_UI.pyi`.
- Fixed `SaveCallbackObjToPikaObj()` to return `True` on success and `False` on failure.
- Removed the extra callback pre-free and let `obj_setArg()` own replacement.
- Made the UI constructor tolerate a missing argument tuple.
- Updated `same_game.py` to use `MatrixOS_UI.UI`, `SetLoopFunc()`, `SetPreRenderFunc()`,
  `PullInput()`, `MatrixOS_UIButton.UIButton`, `SetEnableFunc()`, `SetColorFunc()`, and `OnPress()`.

Follow-up:

- Add a simulator smoke test that runs a Python UI app through WebUI RPC and presses a `UIButton`.
- Keep future Python examples on the UI wrapper path when the goal is API coverage.

## 2026-06-12: Repeated `Point` construction during LED rendering can destabilize WASM

Observed while testing SameGame in MystrixSim.

Small scripts can call:

```python
MatrixOS.LED.SetColor(Point(0, 0), Color(0x00FF00))
```

successfully, but repeatedly constructing many `Point` objects inside a render loop caused intermittent
`memory access out of bounds` failures in the simulator. SameGame hit this path because it rendered all
64 cells every frame with `SetColor(Point(x, y), color)`.

Workaround:

- Use `MatrixOS.LED.SetColorByID(index, color)` for dense framebuffer-style writes.
- Reuse existing `Color` objects where possible.

Fix direction:

- Investigate native-object lifetime and GC interaction for short-lived wrapper objects such as `Point`.
- Add a stress regression that writes all 64 LEDs repeatedly from Python using both `SetColor()` and
  `SetColorByID()`.

## 2026-06-12: UI input callbacks must be polled from Python

Observed while testing SameGame input through a native UI input callback.

Calling Python directly from the native UI input dispatch path is unstable in MystrixSim/PikaPython.
The setter could be installed, but dispatching an input event into Python could trigger WASM `unwind`
failures. The callback body could be as small as:

```python
def handler(code):
    return True
```

Fix:

- `UI` now captures keypad input into its native queue instead of invoking Python from the input
  dispatch stack.
- Python UI loops call `ui.PullInput()` and process pure Python `KeyEvent` objects from normal Python
  loop code.
- Unread queued input is cleared after each UI loop callback.
- SameGame uses this UI wrapper path for board and Function key input, while its settings menu still
  uses `UIButton` callbacks.

Fix direction:

- Keep native-to-Python callbacks for simple UI lifecycle hooks where they already run on the UI loop.
- Prefer queued polling for hot input paths or callbacks that originate in device/input dispatch.
- Add a regression that presses and releases Function Key while a Python UI is active.

## 2026-06-12: Returned closures do not reliably capture locals

Observed while building SameGame board buttons.

Factory-generated callbacks like this failed later with `NameError: name 'index' is not defined`:

```python
def make_color_func(index):
    def color_func():
        return colors[index]
    return color_func
```

Default-argument capture did not fix it in PikaPython.

Fix direction:

- Avoid returned closures in shipped examples.
- Add a small closure/default-argument regression before using CPython closure patterns.
