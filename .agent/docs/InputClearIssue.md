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

## Chosen Solution

The fix uses two explicit, orthogonal APIs:

### `MatrixOS::Input::ClearInputBuffer()`

Discards all pending events from the OS-side input event queue.
Does not touch device-side scan state or the OS snapshot cache.

Previously named `ClearQueue()`.

### `Device::Input::SuppressActiveInputs()`

Marks all currently active device-side input scan states as suppressed
so their follow-up release events are silently dropped by the scan loop.
Does **not** clear the OS input event buffer.
Does **not** clear the OS snapshot cache.

This replaced `Device::KeyPad::Clear()` and operates through the same
`KeypadInfo::Suppress()` mechanism (formerly `KeypadInfo::Clear()`).

### UI transition sequence

At `UI::Start()`, the framework now calls both:

```cpp
MatrixOS::Input::ClearInputBuffer();
Device::Input::SuppressActiveInputs();
```

At `UI::UIEnd()` and `UI::ExitAllUIs()`, only `ClearInputBuffer()` is used
because suppression is only needed when entering a new input context.

### What was removed

- Public `MatrixOS::Input::ClearState()` — removed from the API surface.
  Call sites that used it for cleanup now call `ClearInputBuffer()` directly.

- `Device::KeyPad::Clear()` — removed from the device API surface.
  Its behavior moved to `Device::Input::SuppressActiveInputs()`.

### Snapshot cache invalidation

`ClearInputBuffer()` now also clears the internal `stateCache` map so that
`GetState()` / Python `GetState()` cannot return stale pressed/hold snapshots
from before the context transition.

Without this, when `SuppressActiveInputs()` prevents a release event from
being emitted, the cache would keep the old pressed/hold snapshot forever,
causing `GetState()` to report a suppressed key as still pressed.

The cache clear is internal to the OS input runtime — no new public API was
added. Cache entries are rebuilt naturally as new events arrive via
`NewEvent()`.

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

The current design conflates three different operations:

1. clear pending OS-side input events
2. clear cached OS-side input snapshots
3. consume currently active physical keypad state and suppress its future release

The fix separates these cleanly into `ClearInputBuffer()` (1 + 2) and
`SuppressActiveInputs()` (3). Cache clearing (2) is now handled
internally by `ClearInputBuffer()` so that suppressed keys cannot leave
stale snapshots in `GetState()`.
