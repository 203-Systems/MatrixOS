# Python Refactor TODO

This file is the execution checklist for the new Python binding redesign.

It assumes:

- [PythonRefactorPlan.md](c:/Users/NengzhuoCai/Documents/GitHub/Matrix/MatrixOS/PythonRefactorPlan.md) is the high-level direction
- [PythonRefactorSpec.md](c:/Users/NengzhuoCai/Documents/GitHub/Matrix/MatrixOS/PythonRefactorSpec.md) is the public API design
- [PythonBindingRuntimeSpec.md](c:/Users/NengzhuoCai/Documents/GitHub/Matrix/MatrixOS/PythonBindingRuntimeSpec.md) is the runtime architecture contract

---

## Phase 0: Freeze Scope

- [x] Keep Python disabled in device application lists until the new runtime is ready
- [x] Confirm that old Python `KeyPad / KeyEvent / KeyInfo` APIs are deletion targets
- [x] Confirm that no new work should go into legacy Python compatibility

## Phase 1: Replace Binding Runtime Helpers

- [x] Audit current uses of `createCppObjPtrInPikaObj`
- [x] Audit current uses of `copyCppObjIntoPikaObj`
- [x] Audit current uses of `getCppObjPtrInPikaObj`
- [x] Audit current uses of `SaveCallbackObjToPikaObj`
- [x] Audit current uses of `CallCallbackInPikaObj*`
- [x] Introduce value-wrapper helper API
- [x] Introduce handle-wrapper helper API
- [x] Introduce callback context / lifecycle helper API
- [ ] Delete or retire old unsafe helper paths

## Phase 2: Migrate UI Wrappers To Handle Model

- [x] Rewrite `UI` wrapper to store a native handle instead of raw copied object
- [x] Rewrite `UIComponent` wrapper to store a native handle
- [x] Rewrite `UIButton` wrapper to store a native handle
- [x] Rewrite `UISelector` wrapper to store a native handle
- [x] Rewrite `UI4pxNumber` wrapper to store a native handle if needed
- [x] Add deterministic `close()` or equivalent teardown path
- [x] Ensure callback unregister happens during destroy/close
- [x] Ensure methods on closed wrappers fail safely

## Phase 3: Implement New Input Python API

- [ ] Add `MatrixOS_Input.py`
- [ ] Add `_MatrixOS_Input.pyi`
- [ ] Add new C++ input binding implementation
- [ ] Expose `InputId`
- [ ] Expose `InputClass`
- [ ] Expose `InputEvent`
- [ ] Expose `InputSnapshot` if included in first version
- [ ] Expose `KeypadInfo`
- [ ] Implement `get_event(timeout_ms=0)`
- [ ] Implement `get_state(input_id)`
- [ ] Implement `get_clusters()`
- [ ] Implement `get_primary_grid_cluster()`
- [ ] Implement `get_position(input_id)`
- [ ] Implement `get_inputs_at(point)`
- [ ] Implement `clear_queue()`
- [ ] Implement `clear_state()`
- [ ] Implement `function_key()`

## Phase 4: Implement New UI Python API

- [ ] Replace Python-facing `SetKeyEventHandler` with input-centric API
- [ ] Add `set_input_handler`
- [ ] Expose input events with explicit `input_class`
- [ ] Ensure keypad-specific handling is explicit in Python
- [ ] Remove keypad-era assumptions from Python UI callback path

## Phase 5: Remove Legacy Python API Surface

- [ ] Delete `MatrixOS_KeyPad.py`
- [ ] Delete `MatrixOS_KeyEvent.py`
- [ ] Delete `MatrixOS_KeyInfo.py`
- [ ] Delete `_MatrixOS_KeyPad.pyi`
- [ ] Delete `_MatrixOS_KeyEvent.pyi`
- [ ] Delete `_MatrixOS_KeyInfo.pyi`
- [ ] Remove legacy keypad re-exports from `MatrixOS.py`
- [ ] Remove legacy keypad re-exports from `MatrixOS_Framework.py`

## Phase 6: Regenerate Binding Artifacts

- [ ] Regenerate `pikascript-api/`
- [ ] Verify generated headers match the new `.pyi`
- [ ] Verify no generated file still depends on old keypad-era symbols

## Phase 7: Re-enable Python Incrementally

- [ ] Re-enable Python in app list for one target only
- [ ] Add one smoke test app for input polling
- [ ] Add one smoke test app for UI callback registration
- [ ] Add one smoke test app for value wrappers
- [ ] Verify no immediate hardfault on UI create/destroy loop
- [ ] Verify no immediate hardfault on callback re-registration loop
- [ ] Re-enable Python for remaining targets after smoke tests pass

## Phase 8: App Migration

- [ ] Audit existing Python apps/scripts
- [ ] Replace old keypad imports with new input imports
- [ ] Replace old key event callbacks with new input callbacks
- [ ] Remove dependence on integer key IDs
- [ ] Update examples and templates

## Phase 9: Cleanup

- [ ] Remove dead compatibility comments
- [ ] Remove obsolete wrapper files
- [ ] Remove obsolete generated stubs
- [ ] Write migration notes for future Python scripts
- [ ] Write final Python re-enable checklist

---

## Acceptance Checklist

- [ ] No non-trivial UI object is stored in `PikaObj` by raw struct copy
- [ ] Old Python `KeyPad / KeyEvent / KeyInfo` API is gone
- [ ] Python input API is input-first, not keypad-first
- [ ] Callback lifecycle is explicit and teardown-safe
- [ ] Python can be enabled without known structural hardfault traps
