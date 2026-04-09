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
- [x] Delete or retire old unsafe helper paths

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

- [x] Add `MatrixOS_Input.py`
- [x] Add `_MatrixOS_Input.pyi`
- [x] Add new C++ input binding implementation
- [x] Expose `InputId`
- [x] Expose `InputClass`
- [x] Expose `InputEvent`
- [x] Expose `InputSnapshot` if included in first version
- [x] Expose `KeypadInfo`
- [x] Expose `InputCluster`
- [x] Implement `get_event(timeout_ms=0)`
- [x] Implement `get_state(input_id)`
- [x] Implement `get_clusters()`
- [x] Implement `get_primary_grid_cluster()`
- [x] Implement `get_position(input_id)`
- [x] Implement `get_inputs_at(point)`
- [x] Implement `clear_queue()`
- [x] Implement `clear_state()`
- [x] Implement `function_key()`

## Phase 4: Implement New UI Python API

- [x] Replace Python-facing `SetKeyEventHandler` with input-centric API
- [x] Add `set_input_handler`
- [x] Expose input events with explicit `input_class`
- [x] Ensure keypad-specific handling is explicit in Python
- [x] Remove keypad-era assumptions from Python UI callback path
- [x] Add input-class-check-first usage examples to InputEvent, InputSnapshot, UI stubs

## Phase 5: Remove Legacy Python API Surface

- [x] Delete `MatrixOS_KeyPad.py`
- [x] Delete `MatrixOS_KeyEvent.py`
- [x] Delete `MatrixOS_KeyInfo.py`
- [x] Delete `_MatrixOS_KeyPad.pyi`
- [x] Delete `_MatrixOS_KeyEvent.pyi`
- [x] Delete `_MatrixOS_KeyInfo.pyi`
- [x] Remove legacy keypad re-exports from `MatrixOS.py`
- [x] Remove legacy keypad re-exports from `MatrixOS_Framework.py`
- [x] Gut legacy C++ binding files (`MatrixOS_KeyPad.cpp`, `MatrixOS_KeyEvent.cpp`, `MatrixOS_KeyInfo.cpp`)
- [x] Gut legacy generated headers in `pikascript-api/`
- [x] Remove legacy binding blocks from `__pikaBinding.c`
- [x] Remove legacy `#include` lines from `__pikaBinding.c` (Round 5)
- [x] Neuter legacy `.h` files — empty header guards, no function declarations (Round 5)
- [x] Physically delete gutted legacy files from tree (`git rm`)
- [x] Physically delete `compute_hashes.py` from repo root

## Phase 6: Regenerate Binding Artifacts

- [x] Regenerate `pikascript-api/` (hand-updated; `rust-msc-latest-win10.exe` could not be run — no shell)
- [x] Verify generated headers match the new `.pyi`
- [x] Verify no generated file still depends on old keypad-era symbols
- [x] Re-run Pika compiler (`rust-msc-latest-win10.exe`) to regenerate from .pyi source-of-truth
- [x] After physical file deletion, re-run generator to confirm no legacy symbols reappear

## Phase 7: Re-enable Python Incrementally

Target: Mystrix1 (same hardware/build as Mystrix2; both have Python disabled identically)

### Prerequisites (all met)
- [x] Binding surface is input-first with no legacy keypad symbols
- [x] Pika generator has been run and generated output is consistent
- [x] `.py` and `.pyi` typing is internally consistent (nullable convention applied)
- [x] `SetKeyEventHandler` fully removed; `SetInputHandler` is the only UI input API
- [ ] Build verification on target (blocked: ESP-IDF toolchain version mismatch in dev env)

### Re-enable steps
- [ ] Re-enable Python in `Devices/Mystrix1/ApplicationList.txt` (uncomment `[System]Python`)
- [ ] Verify build succeeds with `idf.py -B build/Mystrix1 -DDEVICE=Mystrix1 build`
- [ ] Add one smoke test script exercising input polling (`MatrixOS.Input.GetEvent()`)
- [ ] Add one smoke test script exercising UI callback registration (`UI.SetInputHandler()`)
- [ ] Add one smoke test script exercising value wrappers (`InputId`, `Point`, `Color`)
- [ ] Verify no immediate hardfault on UI create/destroy loop
- [ ] Verify no immediate hardfault on callback re-registration loop
- [ ] Re-enable Python for Mystrix2 after Mystrix1 smoke tests pass

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
