# MatrixOS Python Runtime

MatrixOS currently embeds MicroPython as the Python engine for the `Python` app.

This directory has two different kinds of MicroPython code:

- `MicroPythonEmbed/` is the vendored embed snapshot that MatrixOS actually builds.
- `MicroPythonPort/` is the MatrixOS-specific port layer: runtime lifecycle, file hooks, port config, and native `MatrixOS` module bindings.

The public Python API is documented in `micropython-api.md`.

## Why Is MicroPython Copied Here?

MatrixOS does not build MicroPython as a standalone upstream port. The `Python` app is built through the normal MatrixOS CMake application flow, so the runtime sources need to be available as normal MatrixOS library sources.

`MicroPythonEmbed/` is therefore a checked-in snapshot of the MicroPython core files needed by the MatrixOS app:

- `py/` contains the MicroPython parser, compiler, VM, GC, object model, and native emitters.
- `extmod/` contains selected MicroPython extension modules.
- `port/` contains MatrixOS embed glue used by the runtime wrapper.
- `genhdr/` contains generated MicroPython headers such as qstr definitions and root pointers.

The generated headers are checked in intentionally. A normal MatrixOS firmware build should not need to run MicroPython's qstr/root-pointer generation tools.

## Why Keep `Library/micropython`?

`Library/micropython` is the upstream MicroPython submodule. The normal MatrixOS firmware build does not compile sources directly from that submodule.

It is kept as the source of truth for regenerating or refreshing `MicroPythonEmbed/`. The regeneration helper lives in `MicroPythonPort/micropython_embed.mk` and expects the upstream tree at `Library/micropython`.

Do not delete `Library/micropython` unless the regeneration workflow is replaced by another pinned upstream source, such as a fetch script or vendored archive with a recorded MicroPython commit.

## Maintenance Rule

Treat `MicroPythonEmbed/` as vendored generated source. Keep normal MatrixOS work in `MicroPythonPort/` whenever possible.

If `MicroPythonEmbed/` must change:

1. Prefer regenerating it from `Library/micropython`.
2. Review generated `genhdr/` changes carefully.
3. Run the MicroPython verification checks documented in `MicroPythonPort/README.md`.

## Uprevving MicroPython

Yes: after updating the `Library/micropython` submodule, `MicroPythonPort/micropython_embed.mk`
is the tool that refreshes the checked-in `MicroPythonEmbed/` snapshot.

Update the upstream submodule first:

```sh
git -C Library/micropython fetch --tags
git -C Library/micropython checkout <tag-or-commit>
```

Then regenerate the embedded package:

```sh
cd Applications/Python/MicroPythonPort
make -f micropython_embed.mk clean
make -f micropython_embed.mk
```

The generator copies upstream embed sources and then overlays files from
`Applications/Python/MicroPythonPort/port/`. After regenerating, review the resulting diff and
reapply any MatrixOS-specific patches that still need to live in `MicroPythonEmbed/`.

Useful checks after an uprev:

```powershell
python Applications\Python\tools\check_micropython_api_surface.py
npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-build --skip-web-build
make build-dev DEVICE=Mystrix1
```

## Current MicroPython Snapshot Patches

These are the intentional MatrixOS differences to watch when comparing `MicroPythonEmbed/`
against `Library/micropython`:

- `MicroPythonEmbed/py/mpstate.h` adds `MP_STATE_PORT(x)` as an alias to `MP_STATE_VM(x)`.
  This keeps MicroPython's persistent-code root-pointer path compatible with the embedded
  MatrixOS port state layout.
- `MicroPythonEmbed/port/embed_util.c` carries the MatrixOS runtime lifecycle changes:
  split-GC heap initialization, ESP32 executable-memory allocation for native code,
  executable chunk cleanup on deinit, the MatrixOS heap split policy hook, and the ESP assert
  guard used by the embedded runtime.
- `MicroPythonEmbed/port/micropython_embed.h` declares the MatrixOS split-heap entry point
  `mp_embed_init_split(...)`.
- `MicroPythonEmbed/port/mphalport.c` routes MicroPython stdout through
  `matrixos_micropython_stdout` instead of upstream's direct `printf` path.
- `MicroPythonEmbed/genhdr/root_pointers.h` includes the persistent-code root pointers needed
  by the enabled native-code features.
- `MicroPythonEmbed/genhdr/qstrdefs.generated.h` is generated output and includes MatrixOS API
  qstrs plus ESP32/native-code qstrs. It should be reviewed after every regeneration, especially
  when usermod bindings or native emitter options change.

Most future MatrixOS-specific changes should go in `MicroPythonPort/` first. If a change must
patch the vendored snapshot, add it to this list so the next MicroPython uprev does not lose it.
