# MatrixOS MicroPython Port

This directory owns the MatrixOS-specific MicroPython runtime glue:

- MicroPython port configuration.
- Runtime lifecycle wrapper used by the `Python` app.
- Lightweight file/import hooks.
- Native `MatrixOS` usermod bindings.

The public Python API is documented in `Applications/Python/micropython-api.md`. This README is for maintainers who need to build, regenerate, or extend the runtime.

## Directory Layout

```text
Applications/Python/MicroPythonPort/
  MicroPythonRuntime.cpp/.h      Runtime init/deinit, script execution, REPL, loop dispatch.
  matrixos_file.cpp              Lightweight MicroPython file/import/open hooks.
  mpconfigport.h                 MatrixOS MicroPython port config.
  micropython_embed.mk           Generates ../MicroPythonEmbed from Library/micropython.
  usermod/matrixos/
    micropython.mk               Explicit usermod source list.
    matrixos_module.cpp          Top-level MatrixOS module registration only.
    matrixos_common.cpp/.h       Shared conversions and callback helpers.
    matrixos_ui.cpp              UI container type and MatrixOS.UI module globals.
    matrixos_ui_components.cpp   Button, Selector, Number, and Toggle bindings.
    matrixos_ui_components.h     Shared UI component base/type declarations.
    matrixos_ui_utility.cpp      Blocking UI utility dialogs.
    matrixos_*.cpp               One binding file per MatrixOS subsystem.
```

`Applications/Python/MicroPythonEmbed` is generated output. Do not hand-edit generated MicroPython sources unless the change is intentionally regenerated and reviewed.

## Regenerating The Embed Package

MicroPython's embed makefile expects POSIX shell tools. On Windows, run this from WSL, MSYS2, Git Bash, or another POSIX-like shell:

```sh
cd Applications/Python/MicroPythonPort
make -f micropython_embed.mk clean
make -f micropython_embed.mk
```

The qstr preprocessor scans the MatrixOS usermod C++ sources. Keep MatrixOS include paths in
`usermod/matrixos/micropython.mk` when a binding starts depending on a new framework or device
header. `build-embed/` is a temporary make output directory and should not be committed.

After regenerating, verify qstr order because MatrixOS keeps `qstrdefs.generated.h` checked in:

```powershell
@'
import re
path='Applications/Python/MicroPythonEmbed/genhdr/qstrdefs.generated.h'
items=[]
for i,line in enumerate(open(path, encoding='utf-8'),1):
    m=re.match(r'QDEF1\(MP_QSTR_([^,]+), [^,]+, [^,]+, "(.*)"\)', line.strip())
    if m:
        items.append((i,m.group(2)))
for a,b in zip(items, items[1:]):
    if a[1] > b[1]:
        print('inversion', a[0], a[1], '>', b[0], b[1])
        raise SystemExit(1)
print('qstr order ok')
'@ | python -
```

The verification runner also performs this check:

```powershell
npm --prefix Devices\MystrixSim\WebUI run verify:micropython
```

## Build And Runtime Verification

Default local verification covers:

- JS syntax checks for WebUI RPC/smoke files.
- `py_compile` for shipped Python examples.
- Pythonic API surface and public API documentation coverage guard.
- qstr order check.
- `MatrixOSHost` build.
- wasm validation.
- WebUI runtime package.
- WebUI production build.

```powershell
npm --prefix Devices\MystrixSim\WebUI run verify:micropython
```

The verification runner sets `EM_CACHE` to `build/emscripten-cache` when the environment does
not provide one. This avoids Emscripten trying to write its cache under `C:\Program Files` on
Windows. If you run `cmake --build build\MystrixSim --target MatrixOSHost --parallel` directly,
set the same cache first:

```powershell
$env:EM_CACHE = (Resolve-Path 'build\emscripten-cache').Path
cmake --build build\MystrixSim --target MatrixOSHost --parallel
```

To run WebUI RPC smoke with an automatically launched dev server and Chrome:

```powershell
npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev
```

Smoke suites can be isolated:

```powershell
npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite core
npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite filesystem
npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite ui
npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite examples
```

## Adding A MatrixOS API

Keep the public API Pythonic. New Python-visible names should use `snake_case`; do not expose PascalCase C++ methods or old Pika compatibility wrappers.

1. Add implementation to the subsystem file, for example `matrixos_led.cpp` or `matrixos_input.cpp`.
   UI container behavior belongs in `matrixos_ui.cpp`; UI component types belong in
   `matrixos_ui_components.cpp`; blocking utility dialogs belong in `matrixos_ui_utility.cpp`.
2. If a new subsystem is needed, create `matrixos_<subsystem>.cpp`.
3. Add the source file to `usermod/matrixos/micropython.mk` and `Applications/Python/CMakeLists.txt`.
4. Add the exported module/type declaration to `matrixos_modules.h`.
5. Register the module/type in `matrixos_module.cpp` only if it is a new top-level export.
6. Add public API documentation to `Applications/Python/micropython-api.md`.
7. Add coverage to `Applications/Python/examples/api_introspection/main.py` or the WebUI RPC smoke suite.
8. Regenerate qstrs if new Python-visible names were added.
9. Run `npm --prefix Devices\MystrixSim\WebUI run verify:micropython`.

`matrixos_module.cpp` should remain a small top-level assembly file. Shared conversions belong in `matrixos_common.cpp`; subsystem behavior belongs in the matching subsystem file.

## Object And Data Rules

Public values should stay simple and predictable:

- Colors are packed RGB integers, accepted from packed ints or RGB/RGBW tuples.
- Points and dimensions are Python tuples.
- Input IDs are Python tuples: `(cluster_id, member_id)`.
- Byte-oriented APIs use `bytes` or byte-compatible objects, not integer lists unless explicitly documented.
- Time values are integer milliseconds unless the API name says otherwise.

If a native C++ object must be wrapped, define a clear owner:

- Python-owned wrapper: release native resources in the type finalizer if one is added.
- Native-owned wrapper: never delete the native object from Python; document lifetime and invalidation.
- UI component wrappers attached to a UI must remain GC-rooted while native UI can call back into Python.

## Callback And GC Rules

UI callbacks cross from native C++ into MicroPython. Treat them as runtime boundaries:

- Store Python callback objects in the wrapper object so MicroPython GC can see them.
- Keep attached UI components alive for at least as long as their native UI is alive.
- Use the shared protected-call helper from `matrixos_common.cpp` so callback exceptions are printed as tracebacks and do not corrupt native UI state.
- Callback exceptions should be visible in WebUI output and covered by RPC smoke.
- Do not hold borrowed `mp_obj_t` values in static storage unless they are explicitly rooted.
- Do not allocate temporary native buffers whose pointers are retained after the binding function returns.

## File And Import Hooks

`matrixos_file.cpp` implements a lightweight path for:

- staged WebUI files under `host:/python/`;
- relative imports from the current script directory;
- built-in `open()` for common text and binary file modes.

This is intentionally not a full MicroPython VFS. If a real VFS is needed later, implement it as a coherent subsystem pass instead of mixing VFS semantics into the current file hooks.

## Current Status

The runtime has reached the current working baseline: it builds through the normal MatrixOS app flow, passes the MicroPython verification gate, and supports the shipped Python example apps.

Remaining work is deferred hardening or product polish rather than unfinished runtime implementation. Keep those notes in `docs/micropython-runtime-phase3.md` so this README stays focused on maintenance workflow.
