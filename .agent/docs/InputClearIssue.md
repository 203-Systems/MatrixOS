# Input Clear Issue Investigation

## Summary

There is a real semantic bug in the current input clear system.

The visible symptom is:

- hold the function key to enter a new UI
- the new UI is supposed to exit on function-key release
- the new UI exits immediately after opening when the user releases the same press

This is not because an old `Released` event remained in the queue.

It happens because the current UI transition clears only the OS-side input
queue, while the device-side keypad scan state is still active. When the user
finally releases the key, the device generates a brand new `Released` event,
and the new UI consumes it as if it were meant for that UI.

---

## Architecture

The OS input layer owns **only** the event queue. All live input state and
keypad capabilities are owned by the device layer.

- `MatrixOS::Input::GetState()` delegates to `Device::Input::GetState()`,
  which reads the current live hardware state directly — no OS-side cache.
- `MatrixOS::Input::GetKeypadCapabilities()` delegates to
  `Device::Input::GetKeypadCapabilities()`, which returns device-defined
  metadata — no OS-side registration map.

There is no `InvalidateStateCache()` because there is no OS-side state cache.

---

## Chosen Solution

The fix uses two explicit, orthogonal mechanisms:

### `MatrixOS::Input::ClearInputBuffer()` — public

Discards all pending events from the OS-side input event queue.
Does **not** touch device-side scan state.

Previously named `ClearQueue()`.

### `Device::Input::SuppressActiveInputs()` — device-side

Marks all currently active device-side input scan states as suppressed
so their follow-up release events are silently dropped by the scan loop.
Does **not** clear the OS input event buffer.

This replaced `Device::KeyPad::Clear()` and operates through the same
`KeypadInfo::Suppress()` mechanism (formerly `KeypadInfo::Clear()`).

### UI transition / rotation sequence

At `UI::Start()` and in `Device::Rotate()`, the framework calls:

```cpp
MatrixOS::Input::ClearInputBuffer();
Device::Input::SuppressActiveInputs();
```

At `UI::UIEnd()` and `UI::ExitAllUIs()`, only `ClearInputBuffer()` is used
because suppression is only needed when entering a new input context.

### Device-owned live state queries

- `Device::Input::GetState(InputId id, InputSnapshot* snapshot)` — returns
  the current live hardware state for any supported input. The OS `GetState()`
  simply delegates here.
- `Device::Input::GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps)`
  — returns device-defined keypad capabilities. The OS `GetKeypadCapabilities()`
  simply delegates here.

### What was removed

- Public `MatrixOS::Input::ClearState()` — removed from the API surface.
  Call sites that used it for cleanup now call `ClearInputBuffer()` directly.

- `Device::KeyPad::Clear()` — removed from the device API surface.
  Its behavior moved to `Device::Input::SuppressActiveInputs()`.

- OS-owned `stateCache` — removed. `GetState()` now reads device-owned live
  state directly.

- OS-owned `keypadCapsMap` and `RegisterKeypadCapabilities()` — removed.
  `GetKeypadCapabilities()` now queries the device layer directly.

- `InvalidateStateCache()` — removed (no cache to invalidate).

### `KeypadInfo::Suppress()` semantics

`Suppress()` now covers all states that `Active()` considers active,
including `ReleaseDebouncing`.  Previously, a key in release-debounce
could still emit a follow-up `Released` event after suppression.  This
is now fixed: any active state, including `ReleaseDebouncing`, sets the
`suppressed` flag.

### Naming consistency

- `KeypadInfo::Clear()` → `KeypadInfo::Suppress()`
- `KeypadInfo::cleared` → `KeypadInfo::suppressed`
- `KeypadInfo::Cleared()` → `KeypadInfo::Suppressed()`

### Rejected alternatives

- `PrepareForUITransition()` — not used; the two explicit calls are clearer
- `ConsumeActiveKeypadStates()` — renamed to `SuppressActiveInputs()` for
  clarity; "suppress" better describes the device-side flag mechanism

---

## Root Cause (unchanged from original analysis)

The original design conflated three different operations:

1. clear pending OS-side input events
2. clear cached OS-side input snapshots
3. consume currently active physical keypad state and suppress its future release

The fix separates these into `ClearInputBuffer()` (1) and
`SuppressActiveInputs()` (3). Operation (2) is no longer needed because the
OS no longer caches input state — it queries device-owned live state directly.
Context-transition call sites invoke both (1) and (3). Buffer-only call sites
(e.g. `UIEnd()`, `ExitAllUIs()`, `LEDTester`) use only `ClearInputBuffer()`.
