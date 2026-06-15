# MicroPython Runtime Phase 3 Status

This document used to track the Phase 3 hardening checklist for the MatrixOS
MicroPython runtime. The implementation has now reached the intended working
baseline: the runtime is usable, the public API is documented and checked, and
the Python example apps exercise the main MatrixOS subsystems.

There are no known half-finished Python runtime features that must be completed
before normal development can continue. The remaining items below are deferred
hardening or product-polish work, not active blockers.

## Current Baseline

- `Applications/Python/micropython-api.md` is the public API contract.
- `Applications/Python/examples/api_introspection/main.py` and
  `Applications/Python/tools/check_micropython_api_surface.py` guard the API
  surface and documentation drift.
- The MicroPython runtime is split into a MatrixOS-specific port layer
  (`Applications/Python/MicroPythonPort/`) and a vendored MicroPython snapshot
  (`Applications/Python/MicroPythonEmbed/`).
- The normal MatrixOS build uses the checked-in vendored snapshot; MicroPython
  uprev details are documented in `Applications/Python/README.md`.
- Python examples are packaged as app folders with `AppInfo.json`.
- Pixel Art, SameGame, Gomoku, Dice, Lighting, and Reversi have Python examples
  and WebUI/MystrixSim smoke coverage for their main flows.
- UI wrappers cover the current app needs: `UI`, `Button`, `Selector`,
  `Number`, `Toggle`, `CustomComponent`, `text_scroll`, `color_picker`, and
  `number_selector`.
- Runtime lifecycle, exception reporting, split GC heap allocation, file/import
  hooks, and core MatrixOS subsystem bindings have smoke-test coverage.

## Verification Gate

Use this as the normal local regression gate after Python runtime or example
changes:

```powershell
npm --prefix Devices\MystrixSim\WebUI run verify:micropython
```

For faster iteration when the simulator and WebUI build artifacts are already
current:

```powershell
npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-build --skip-web-build
```

For browser-driven smoke against a single example:

```powershell
npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-build --skip-web-build --smoke-dev --suite examples --example lighting
```

## Deferred Work

These are useful future improvements, but they are intentionally not treated as
open implementation tasks right now.

### Device Readiness Polish

- Record per-family heap and stack tuning guidance.
- Document script storage policy per device family.
- Document stdout/stdin expectations for USB CDC, hardware logs, and WebUI.
- Keep at least one real-device validation note current when device behavior
  changes.

### Debugging UX

- Decide the final product relationship between `print()` and structured
  MatrixOS logging.
- Improve WebUI display for stdout, traceback, last exception, active app
  status, and runtime memory summary.
- Add richer callback source labels in exception output if Python callback
  debugging becomes painful.

### Stress Coverage

- Add longer-running stress smoke for repeated imports.
- Add repeated UI enter/exit stress tests.
- Add high-volume LED write stress tests.
- Add longer real-device soak tests if heap fragmentation or callback lifetime
  issues reappear.

### Packaging Documentation

- Keep multi-file app layout and import behavior documented as the WebUI Python
  editing UX evolves.
- Expand FileSystem examples if packaged Python apps start relying on more than
  the current common `open()` and import paths.

## Historical Notes

Older Phase 1 and Phase 2 planning docs remain in `docs/` as migration history.
They may mention unfinished work that has since been completed, superseded, or
deferred. Treat this file, `Applications/Python/README.md`, and
`Applications/Python/micropython-api.md` as the current source of truth.
