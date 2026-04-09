# Python API Migration Notes

This document describes how to migrate Python scripts from the old
keypad-centric Python API to the new input-first Python API in MatrixOS.

---

## Summary of Changes

| Old API                | New API                                    |
|------------------------|--------------------------------------------|
| `MatrixOS.KeyPad`      | `MatrixOS.Input`                           |
| `KeyEvent`             | `InputEvent`                               |
| `KeyInfo`              | `KeypadInfo` (accessed via `InputEvent`)   |
| `KeyPad.GetKey()`      | `Input.GetEvent(timeout_ms)`               |
| `KeyPad.GetKeyState()` | `Input.GetState(input_id)`                 |
| `UI.SetKeyEventHandler(cb)` | `UI.SetInputHandler(cb)`              |
| Integer key index      | `InputId` (cluster + member)               |

---

## Core Concepts

### InputId

Every input is identified by an `InputId`, which carries a **cluster ID** and
a **member ID**.  There is no single integer key index any more.

```python
from MatrixOS_InputId import InputId

fk = InputId.FunctionKey()   # The device function key
inv = InputId.Invalid()      # Sentinel value

print(fk.ClusterId(), fk.MemberId())
print(fk == fk)   # True
print(fk != inv)   # True
```

### InputClass

Not every input is a keypad.  The `InputClass` constants tell you what kind of
input generated an event.

```python
import MatrixOS_InputClass as InputClass

# Constants:
#   InputClass.UNKNOWN
#   InputClass.KEYPAD
#   InputClass.FADER
#   InputClass.ENCODER
#   InputClass.TOUCH_AREA
#   InputClass.GYRO
#   InputClass.ACCELEROMETER
#   InputClass.TEMPERATURE
#   InputClass.BATTERY
#   InputClass.GENERIC
```

**Always check `InputClass` before accessing payload-specific data.**

### InputEvent

Returned by `MatrixOS.Input.GetEvent()`.

```python
event = MatrixOS.Input.GetEvent(100)  # 100 ms timeout
if event is not None:
    input_id = event.Id()         # InputId
    cls = event.InputClass()      # int (InputClass constant)

    if cls == InputClass.KEYPAD:
        info = event.Keypad()     # KeypadInfo or None
        if info is not None:
            print(info.State(), info.Force())
```

### InputSnapshot

Returned by `MatrixOS.Input.GetState()`.  Same shape as `InputEvent`.

```python
snap = MatrixOS.Input.GetState(some_input_id)
if snap is not None and snap.InputClass() == InputClass.KEYPAD:
    info = snap.Keypad()
    if info is not None:
        print(info.State(), info.Force(), info.Hold())
```

### KeypadInfo

Keypad-specific state extracted from `InputEvent.Keypad()` or
`InputSnapshot.Keypad()`.

| Method           | Returns | Description                           |
|------------------|---------|---------------------------------------|
| `State()`        | `int`   | `KeyState` enum value                 |
| `Force()`        | `float` | Pressure (0.0–1.0)                    |
| `Value(index)`   | `float` | Generic axis (0 = pressure, 1 = vel.) |
| `Active()`       | `bool`  | Key is in an active state             |
| `Hold()`         | `bool`  | Key is being held                     |
| `HoldTime()`     | `int`   | Hold duration in ms                   |
| `LastEventTime()`| `int`   | Timestamp of last state change (ms)   |

### KeyState

```python
from MatrixOS_KeyState import KeyState

# KeyState.IDLE         = 0
# KeyState.ACTIVATED    = 1
# KeyState.PRESSED      = 2
# KeyState.HOLD         = 3
# KeyState.AFTERTOUCH   = 4
# KeyState.RELEASED     = 5
```

---

## Polling for Events

```python
import MatrixOS
import MatrixOS_InputClass as InputClass
from MatrixOS_KeyState import KeyState

# Non-blocking poll
event = MatrixOS.Input.GetEvent(0)

# Blocking poll with 1-second timeout
event = MatrixOS.Input.GetEvent(1000)

if event is not None:
    if event.InputClass() == InputClass.KEYPAD:
        info = event.Keypad()
        if info is not None:
            if info.State() == KeyState.PRESSED:
                print("Key pressed! Force:", info.Force())
```

---

## Reading Current State

```python
import MatrixOS
import MatrixOS_InputClass as InputClass

fk = MatrixOS.Input.FunctionKey()

snap = MatrixOS.Input.GetState(fk)
if snap is not None and snap.InputClass() == InputClass.KEYPAD:
    info = snap.Keypad()
    if info is not None:
        print("Function key state:", info.State())
```

---

## UI Callback Migration

### Old pattern (deleted)

```python
# DO NOT USE — this API no longer exists
def on_key(key_event):
    key_id = key_event.GetId()
    ...

ui.SetKeyEventHandler(on_key)
```

### New pattern

```python
import MatrixOS_InputClass as InputClass
from MatrixOS_UI import UI
from MatrixOS_Color import Color

def on_input(event):
    cls = event.InputClass()
    if cls != InputClass.KEYPAD:
        return False  # not consumed — let framework handle

    info = event.Keypad()
    if info is None:
        return False

    # Use info.State(), info.Force(), event.Id(), etc.
    cid = event.Id().ClusterId()
    mid = event.Id().MemberId()
    print("key", cid, mid, "state", info.State())
    return True  # consumed

ui = UI("MyApp", Color(0x00FF00))
ui.SetInputHandler(on_input)
ui.Start()
ui.Close()
```

The handler receives an `InputEvent` and must return `bool`:
- `True` = event consumed
- `False` = event not consumed (propagated to framework default handling)

---

## Querying Input Topology

```python
import MatrixOS

# All clusters
clusters = MatrixOS.Input.GetClusters()
for c in clusters:
    print(c.ClusterId(), c.Name(), c.InputClass(), c.InputCount())

# Primary grid cluster (the main keypad grid)
grid = MatrixOS.Input.GetPrimaryGridCluster()
if grid is not None:
    dim = grid.Dimension()
    print("Grid size:", dim.X(), "x", dim.Y())

# Position round-trip
pos = MatrixOS.Input.GetPosition(some_id)
ids = MatrixOS.Input.GetInputsAt(pos)
```

---

## Clearing Input State

```python
import MatrixOS

MatrixOS.Input.ClearQueue()  # discard queued events
MatrixOS.Input.ClearState()  # reset pressure / hold state
```

---

## Value Wrapper Guidance

`InputId`, `Point`, `Color`, `InputCluster`, and `KeypadInfo` are all
value wrappers.  They are lightweight Python objects backed by C++ data.
You may store them, compare them, and pass them around freely.

UI objects (`UI`, `UIButton`, `UISelector`, `UI4pxNumber`) are **handle
wrappers**.  They own a native C++ resource and must be closed explicitly
with `.Close()` when done.  Methods called on a closed handle return
safely (no crash) but do nothing.

---

## Quick Reference: Imports

```python
import MatrixOS                              # MatrixOS.Input, MatrixOS.SYS, etc.
from MatrixOS_Framework import *             # Point, Color, InputEvent, etc.

import MatrixOS_InputClass as InputClass     # InputClass.KEYPAD, etc.
from MatrixOS_KeyState import KeyState       # KeyState.PRESSED, etc.
from MatrixOS_InputId import InputId         # InputId.FunctionKey(), etc.

from MatrixOS_UI import UI                   # UI, UIButton, UISelector
from MatrixOS_Color import Color             # Color(0xRRGGBB)
from MatrixOS_Point import Point             # Point(x, y)
```
