# MatrixOS MicroPython Runtime Port

This note records the first MicroPython runtime cut for MatrixOS.

## Runtime Shape

- `Applications/Python` embeds MicroPython through `Applications/Python/MicroPythonEmbed`.
- `Applications/Python/MicroPythonPort` owns MatrixOS-specific port config, runtime lifecycle, stdout bridge, and the native `MatrixOS` module.
- `Library/micropython` is pinned as a submodule at MicroPython `v1.28.0`.
- The active app lifecycle is:
  - `Python::Setup()` initializes MicroPython.
  - Script mode executes one `.py` source.
  - If the script defines `loop()`, MatrixOS calls it once per app loop.
  - If no `loop()` exists, the app exits on the next app loop, not during `Setup()`.
  - REPL mode keeps MicroPython alive until Ctrl-D or `MatrixOS.SYS.exit_app()`.

## Public Python API

The active MicroPython-facing API is intentionally Pythonic:

- `import MatrixOS`
- `MatrixOS.Color(...)`
- `MatrixOS.Timer`
- `MatrixOS.SYS`
- `MatrixOS.LED`
- `MatrixOS.Input`
- `MatrixOS.ColorEffects`
- `MatrixOS.NVS`
- `MatrixOS.Utils`
- `MatrixOS.Logging`
- `MatrixOS.FileSystem`
- `MatrixOS.USB`
- `MatrixOS.HID`
- `MatrixOS.MIDI`
- `MatrixOS.UI`

Public names use `snake_case`; old Pika-style PascalCase wrappers are not part of the active API.
The old Pika-style generated facade has been removed from the tree and is not part of the active build path.

## API Surface Status

Native UI, MIDI, USB, HID, FileSystem, and Logging wrappers have moved past the first-cut deferred state and now have MicroPython bindings plus WebUI smoke coverage.

- The four shipped examples launch under the MicroPython runtime.
- Native UI wrappers are available for setting-style UI flows.
- MIDI, USB, HID, FileSystem, and Logging bindings are designed from current MatrixOS C++ contracts, not copied from the old Pika facade shape.
- Remaining production work is app behavior parity, UI edge-case parity, logging/exception policy, and device-family tuning.

## Script Loading

MatrixOS loads the main Python script as a complete source string before executing it.

- On MystrixSim/WebUI, the RPC bridge stages script text through `MystrixSim::HostIO`.
- On storage-enabled devices, `Python::ReadPythonFile()` reads the script path through `MatrixOS::FileSystem::Open()`.
- Shell launch continues to use `MatrixOS::SYS::ExecuteAPP("203 Systems", "Python", args)`.

MystrixSim/WebUI staging also supports multiple files under `host:/python/`, with one staged entry script and sibling helper modules importable through normal Python import.

## Import And File I/O Strategy

External Python imports and built-in `open()` are enabled through lightweight MatrixOS MicroPython port hooks. This is not a full MicroPython VFS object model; it is the app/script loading and common file I/O path needed for MatrixOS Python apps.

Current config:

- `MICROPY_ENABLE_EXTERNAL_IMPORT = 1`
- `MICROPY_HAS_FILE_READER = 1`
- `MICROPY_PY_IO = 1`
- `MICROPY_VFS = 0`
- `MICROPY_READER_POSIX = 0`

Implemented hooks:

- `mp_import_stat()`
- `mp_reader_new_file()`
- `mp_builtin_open()`
- Current script directory injection into `sys.path`

Supported behavior:

- MystrixSim staged files are stored under `host:/python/`.
- Relative imports resolve from the current script directory.
- Relative `open()` paths resolve from the current script directory.
- `open()` supports common text read/write modes, context manager behavior, `read`, `write`, `seek`, `tell`, `flush`, and `close`.
- WebUI RPC smoke covers importing a staged helper module and reading/writing a small file.

Future full VFS work, if needed, should be a separate coherent pass rather than mixed into the lightweight app file hooks.

## Runtime Debugging

MystrixSim exposes a WebUI JSON-RPC debug endpoint for runtime diagnostics:

```js
await matrixosRpc.call('python.debug')
```

The result includes Python app activity, session mode, loop state, and a MicroPython runtime memory
summary while the runtime is active:

- `scriptPath`
- `scripts.entry`
- `scripts.files[]` with `path`, `size`, and `entry`
- `runtime.initialized`
- `runtime.heapSize`
- `runtime.heapTotal`
- `runtime.heapUsed`
- `runtime.heapFree`
- `runtime.heapMaxFree`
- `runtime.cStackUsage`

`scripts.files[]` reports staged package metadata only; it does not include script contents.
`python.status` also includes the same debug payload under `debug` so smoke-test failure artifacts can
capture runtime memory and staged-file state without making an additional call. This endpoint is
WebUI/debug tooling; it is not part of the in-app `MatrixOS` Python API.

## Native/Device Port Story

The C++ runtime wrapper is not MystrixSim-only. The same `Python` app code initializes MicroPython on device builds.

Device-specific considerations before enabling broadly:

- Heap size is currently fixed in `MicroPythonRuntime` at `96 * 1024` bytes.
- Stack integration uses `mp_stack_ctrl_init()` and the current stack pointer at runtime init.
- Device stdout/stdin default to USB CDC through weak `matrixos_python_*` hooks.
- Storage-enabled devices can load the top-level `.py` script through `MatrixOS::FileSystem`.
- External imports and Python `open()` are available through the lightweight MatrixOS port hooks.
- Target owners should tune heap size and script storage policy per device family before shipping Python as a user-visible app.

## WebUI Verification

The reusable WebUI RPC smoke test is:

```powershell
cd Devices\MystrixSim\WebUI
npm run smoke:micropython -- --ws ws://localhost:4012
```

It expects a running WebUI dev server and an open MystrixSim runtime tab. The smoke currently verifies:

- `python.runText`
- REPL, including pasted multiline input
- `python.subscribe`
- input events through `MatrixOS.Input.get_event()`
- USB CDC, MIDI, RawHID no-device/injection paths
- `api_introspection.py`
- multi-file staged import and `open()` read/write
- native UI interaction and function-key behavior
- lifecycle start/stop, exception recovery, and `python.debug` runtime memory summary
- `pixel_art.py`
- `same_game.py`
- `gomoku.py`
- `dice.py`
