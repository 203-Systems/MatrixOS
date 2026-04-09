# Post Input System Actions

## Goal
Clean up the current input-system API after the main migration, remove remaining keypad-centric helpers and naming, then refactor the Python app and bindings around the new input model.

## Core API Cleanup
- Remove `Dimension GetPrimaryGridSize()` from `MatrixOS::Input`.
- Replace all `GetPrimaryGridSize()` call sites with:
  - `const InputCluster* cluster = MatrixOS::Input::GetPrimaryGridCluster();`
  - read `cluster->dimension` directly.
- Remove `KeypadInfo GetKeypadState(Point xy)`.
- Replace all `GetKeypadState(Point)` usage with explicit input lookup:
  - resolve `InputId` from cluster/position
  - call `GetState(id, &snapshot)`
  - verify `snapshot.inputClass == InputClass::Keypad`
  - then use `snapshot.keypad`
- Remove `bool HasVelocitySensitivity()`.
- Replace all `HasVelocitySensitivity()` usage with explicit capability lookup on the primary grid cluster:
  - `GetPrimaryGridCluster()`
  - `GetKeypadCapabilities(clusterId, &caps)`
  - use `caps.hasVelocity`
- Rename `bool TryGetPoint(InputId id, Point* xy)` to `bool GetPosition(InputId id, Point* xy)`.
- Update all call sites to the new `GetPosition(...)` name.

## Event Handling Rules
- Audit every `MatrixOS::Input::Get(&inputEvent)` loop in applications and OS helpers.
- Require explicit `inputClass` filtering before keypad-style handling.
- For keypad-only code, gate with:
  - `if (inputEvent.inputClass != InputClass::Keypad) continue;`
- Where discard is not correct, branch by `inputClass` explicitly.
- Remove silent assumptions that all input events are keypad events.

## Application Cleanup
- Audit all app loops of the form:

```cpp
while (MatrixOS::Input::Get(&inputEvent))
{
  KeyEventHandler(inputEvent);
}
```

- For each app:
  - decide whether it is keypad-only or multi-input aware
  - add explicit filtering before dispatch
  - rename handlers if they are no longer keypad-specific
- Reduce remaining keypad-centric naming where practical:
  - `KeyEventHandler` -> `InputEventHandler` where the handler is no longer keypad-only
  - keep keypad-specific naming only where the function truly only handles keypad payloads

## UI Framework Cleanup
- Rename `SetKeyEventHandler(...)` to `SetInputEventHandler(...)`.
- Rename internal storage and member names from `key_event_handler` to `keyEventHandler`.
- Rename any remaining keypad-centric UI entry points that are now generic input handlers.
- Require UI runtime to check `inputClass` before keypad-specific dispatch.
- Keep keypad-only component callbacks gated behind keypad checks.
- Audit `UI.h` and `UI.cpp` for:
  - `key_event_handler`
  - `SetKeyEventHandler`
  - keypad-only assumptions in generic input flow

## Input API Consistency
- Review all helper APIs in `MatrixOS::Input` and remove convenience wrappers that duplicate cluster/state access patterns unnecessarily.
- Prefer:
  - `GetCluster(...)`
  - `GetPrimaryGridCluster()`
  - `GetState(...)`
  - `GetInputAt(...)`
  - `GetInputsAt(...)`
  - `GetPosition(...)`
- Avoid reintroducing keypad-era convenience APIs unless they are clearly temporary.

## Python App Phase
- After the C++ post-migration cleanup above is complete, start Python app refactor.
- Re-enable or revisit the Python app only after the new input API is stable.
- Implement Python-facing wrappers for the new input system.
- Remove old keypad-era Python API surface.
- Stop exposing keypad-only concepts as the primary Python model.

## Python Refactor Report
- Create `PythonRefactorReport.md`.
- The report must include:
  - current Python wrapper surface inventory
  - current keypad-era APIs still exposed
  - gaps between current Python API and new input-system design
  - proposed new Python input API
  - compatibility break analysis
  - phased migration path
  - files/modules to delete
  - files/modules to rewrite
  - binding generation implications
  - recommended order of implementation

## Execution Order
1. Remove `GetPrimaryGridSize()`.
2. Remove `GetKeypadState(Point)`.
3. Remove `HasVelocitySensitivity()`.
4. Rename `TryGetPoint(...)` to `GetPosition(...)`.
5. Audit and fix all `MatrixOS::Input::Get(&inputEvent)` loops to check `inputClass`.
6. Rename UI framework APIs and members from key-centric names to input-centric names.
7. Clean up application handler naming where appropriate.
8. Revisit Python app and bindings.
9. Write `PythonRefactorReport.md`.

## Acceptance Criteria
- No remaining use of:
  - `GetPrimaryGridSize()`
  - `GetKeypadState(Point)`
  - `HasVelocitySensitivity()`
  - `TryGetPoint(...)`
  - `SetKeyEventHandler(...)`
- All `MatrixOS::Input::Get(&inputEvent)` consumers explicitly check `inputClass` before keypad-specific processing.
- UI framework uses input-centric naming for generic handlers.
- Python refactor work starts only after the C++ API cleanup is done.
- `PythonRefactorReport.md` exists and documents the Python migration plan in detail.

## TODO
- [x] Remove `MatrixOS::Input::GetPrimaryGridSize()`.
- [x] Replace all `GetPrimaryGridSize()` call sites with `GetPrimaryGridCluster()->dimension`.
- [x] Remove `MatrixOS::Input::GetKeypadState(Point)`.
- [x] Replace all `GetKeypadState(Point)` call sites with explicit `GetInputAt/GetState` flow and `inputClass` checks.
- [x] Remove `MatrixOS::Input::HasVelocitySensitivity()`.
- [x] Replace all `HasVelocitySensitivity()` call sites with explicit keypad capability lookup.
- [x] Rename `MatrixOS::Input::TryGetPoint(...)` to `MatrixOS::Input::GetPosition(...)`.
- [x] Update all `TryGetPoint(...)` call sites to `GetPosition(...)`.
- [x] Audit every `while (MatrixOS::Input::Get(&inputEvent))` loop.
- [x] Require explicit `inputClass` filtering before keypad-specific event handling.
- [x] Rename `UI::SetKeyEventHandler(...)` to `UI::SetInputEventHandler(...)`.
- [x] Rename remaining generic UI handler members from `key_event_handler` to `inputEventHandler`.
- [x] Audit `UI.h` and `UI.cpp` for keypad-centric generic handler naming.
- [x] Rename application-level generic handlers from `KeyEventHandler` to `InputEventHandler` where appropriate.
- [x] Keep keypad-specific names only where the code is truly keypad-only.
- [ ] Revisit Python app after the C++ API cleanup is complete.
- [ ] Implement Python wrappers for the new input-system API.
- [ ] Remove old keypad-era Python API surface.
- [ ] Analysis the current python API wrapper. It's not ideal, stable, safe, or designed well so we need a redesign.
- [ ] Write `PythonRefactorReport.md`.
