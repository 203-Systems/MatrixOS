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
