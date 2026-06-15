# MatrixOS Python API Checklist

This checklist tracks the Python facade quality of `Applications/Python/PikaPython`.

## Design Rules

- Keep native `_MatrixOS_*` modules thin and stable.
- Keep PascalCase names only in private `_MatrixOS_*` native bindings.
- Expose Pythonic public endpoint names for app code.
- Prefer Python facade objects or view objects for structured data.
- Avoid `@property` until PikaPython property behavior is proven stable.
- Avoid returning fresh native objects from hot callback paths when a small primitive bridge is enough.
- Keep high-level examples on UI-owned input queues instead of the global OS input queue.

## LED

Status: mostly reviewed.

- [x] Lowercase helpers for common operations.
- [x] `LEDPartition` facade object exists.
- [x] Partition lookup by index and name.
- [x] Partition metadata: `index`, `name`, `start`, `size`, `type`, `default_multiplier`.
- [x] Convenience helpers: `end()`, `contains()`, `is_rgb()`, `is_rgbw()`.
- [x] Avoid `__repr__` for now. PikaPython dunder behavior is not worth relying on in app examples.
- [x] Keep partition actions as module helpers. Runtime smoke showed object action methods are easier
  to misparse than data-only methods.
- [x] Runtime smoke on fresh WASM page after cache reset.

## Input

Status: mostly reviewed.

- [x] `InputId(cluster_id, member_id)` can be constructed from Python.
- [x] Pythonic `InputIdView`, `InputEventView`, `InputSnapshotView`, and `InputClusterView`.
- [x] `keypad_clusters()`, `keypad_cluster()`, `primary_grid()`, and `get_cluster()`.
- [x] UI `KeyEvent.id()` returns an `InputId`.
- [x] UI `KeyEvent.is_grid()`, `x()`, and `y()`.
- [x] `InputId.member_id()` is the single name for the cluster-local member; duplicate `input_id()`
  aliases were removed.
- [x] `InputId.cluster_name()` and `InputEvent.cluster_name()`.
- [x] `Input.try_get_point()` returns point-or-`None` for coordinate-capable inputs.
- [x] Keypad wrappers expose `pressure()` and `velocity()`.
- [x] Examples should prefer `ui.pull_input()`.
- [x] Add runtime regression for UI pull input with injected keypad events.
- [x] Do not add a separate non-UI input queue service yet. Keep new apps on UI-owned input until a
  real non-UI app needs it.

## UI

Status: reviewed for current native surface.

- [x] UI components are exposed as `MatrixOS.UI.Button`, `MatrixOS.UI.Selector`,
  `MatrixOS.UI.Number`, and `MatrixOS.UI.Component`.
- [x] `UI.pull_input()` returns a pure Python `KeyEvent` or `None`.
- [x] Audit callback lifetime behavior for all component callback setters.
- [x] Add tests for selector and number component value changes.
- [x] Add an example that uses normal `Button.on_press()` plus `ui.pull_input()` together.

## MIDI

Status: reviewed for current native surface.

- [x] Basic lowercase helpers: `get()`, `send()`, `send_sysex()`.
- [x] `MidiPacket` has lowercase getter/setter helpers for directly constructed packets.
- [x] Module-level packet field helpers exist for packets returned by native factories, such as
  `MatrixOS.MIDI.velocity(packet)`.
- [x] Add constructors such as `note_on()`, `note_off()`, `cc()`, `program_change()`, `pitch_bend()`.
- [x] Add packet field helpers for status/channel/data bytes if native API exposes them.
- [x] Review MIDI port constants. Current `MidiPortID` is only a class of constants.
- [x] Add module-level constants for `MidiPortID`.
- [x] Replace illegal `MidiStatus.None` with `MidiStatus.NONE` and module-level constants.
- [x] Add tests for `send_sysex()` length hiding.
- [x] Add tests for packet factory helpers.

## USB

Status: reviewed for current native surface.

- [x] `connected()` alias exists.
- [x] Check whether native USB exposes more state than `Connected()`.
- [x] If available, add device mode/state helpers.
- [x] Add tests for alias behavior.

## HID

Status: reviewed for current native surface.

- [x] Submodules are exposed under `MatrixOS.HID`.
- [x] Add `ready()` lowercase alias to `MatrixOS_HID`.
- [x] Keyboard needs lowercase aliases: `tap()`, `press()`, `release()`, `release_all()`.
- [x] Mouse needs lowercase aliases and default `wheel=0`.
- [x] Touch needs lowercase aliases and default `wheel=0`.
- [x] Gamepad needs lowercase aliases and clearer axis names.
- [x] Consumer/System controls need lowercase aliases.
- [x] RawHID should expose `get()` and `send()` lowercase helpers consistently.
- [x] Review return values. Some submodules return `bool`, some `None`; align with native behavior or document it.
- [x] Add tests for length-hiding RawHID send.

## NVS

Status: reviewed for current native surface.

- [x] Lowercase helpers: `get_size()`, `get()`, `set()`, `delete()`.
- [x] Consider typed helpers: `get_u8`, `set_u8`, `get_u32`, `set_u32`, `get_str`, `set_str`.
- [x] Decide whether byte conversion belongs in the Python facade or app code.
- [x] Add tests for length-hiding `set()`.

## SYS

Status: reviewed for current native surface.

- [x] Lowercase helpers for sleep, time, app launch, reboot, bootloader, settings, and version.
- [x] Avoid a plain `sleep()` alias for now; keep explicit `sleep_ms()` / `delay_ms()` naming.
- [x] Consider a version facade object instead of raw tuple if version shape is stable.
- [x] Add tests for default `args=[]` behavior without sharing mutable defaults.

## Geometry and Color

Status: mostly reviewed.

- [x] `Point` helpers and module constructor.
- [x] `Dimension` helpers and module constructor.
- [x] `Color` helpers and constructors.
- [x] `ColorEffects` lowercase aliases.
- [x] Avoid `__repr__` until PikaPython dunder behavior is proven stable.
- [x] Add more tests for mutation helpers.

## Utils

Status: reviewed for current native surface.

- [x] `string_hash()` alias exists.
- [x] Check native utilities for more exposed functions.
- [x] Add tests for `string_hash()`.

## Constants and Aggregates

Status: reviewed for current native surface.

- [x] Constant modules import under CPython.
- [x] Representative values are tested.
- [x] `MatrixOS_Framework` aggregate exports are tested.
- [x] Every `MatrixOS_*.py` facade module is referenced by at least one test file.
- [x] AST audit shows every facade function/method name is referenced by tests.
- [x] Standard-library `trace` audit shows no facade module below 90% line coverage after filtering docstrings.

## Examples

Status: in progress.

- [x] `pixel_art.py` migrated to UI-owned input.
- [x] `same_game.py` should use the final UI/input facade instead of workaround paths.
- [x] Add a small API demo for LED partitions and keypad clusters.
- [x] Add smoke scripts that can run in MystrixSim RPC once fresh-runtime selection is reliable.

## Runtime Notes

- [x] MystrixSim smoke verified Pythonic LED partition APIs (`LED.partitions()` and
  `LED.get_partition(index_or_name)`), `Input.primary_grid()`,
  object facade methods, and a simple Python script launch through `python.stage` plus
  `python.runStaged`.
- [x] MystrixSim smoke found a Pika parser limitation when multiple object method calls are packed
  into one expression. The issue is documented in `docs/pikapython-issues.md`.
- [x] `python.runText` and very short `python.runStaged` scripts return successfully after the remote
  runner observes app mode, new Python output, or a completed short run.
