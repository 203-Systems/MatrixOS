# MatrixOS Agent Guide

This guide is for future AI agents and engineers modifying MatrixOS.
If this guide and the code disagree, trust the code first.

## References

- Public docs: <https://matrix.203.io/>
- Use a local `Matrix-Wiki` clone when changing public APIs or user-facing behavior.

## Repo Model

MatrixOS is split into three layers:

1. `Devices/`
   Hardware layer. Board-specific drivers, storage, keypad scanning, LEDs, USB, and chip-family details.
2. `OS/`
   Runtime layer. Public `MatrixOS::` APIs, app lifecycle, UI, HID/MIDI, logging, storage wrappers, shared framework code.
3. `Applications/`
   App layer. Apps derive from `Application` and use `MatrixOS::` APIs.

Change the right layer:

- hardware behavior or board capability: `Devices/`
- shared runtime or public OS behavior: `OS/`
- product behavior inside one app: `Applications/`

## Start Here

Read these first for structural work:

- `OS/MatrixOS.h`
- `Devices/Device.h`
- `Applications/Application.h`
- `OS/System/System.cpp`
- `OS/System/Parameters.h`

## Runtime Notes

- Only one app is active at a time.
- Apps run in a dedicated FreeRTOS task.
- App lifecycle is owned by the supervisor.
- App memory is managed manually.
- Do not manually kill app tasks unless you are intentionally changing lifecycle internals.

## Registration

- Apps are registered through CMake and generated files.
- Do not hand-edit generated registration outputs.
- Update the source CMake and family `ApplicationList.txt` instead.

## API Boundary

- Treat `OS/MatrixOS.h` as the app-facing contract.
- Prefer `MatrixOS::...` wrappers over `Device::...` from application code.
- Direct `Device::...` calls are usually only appropriate in device code, OS internals, or carefully justified hot paths.

## Persistence

Two main persistence mechanisms:

- NVS for small settings and compact state
- FileSystem for larger structured data or user content

Rules:

- keep one clear source of truth
- define precedence if multiple backends can hold the same data
- avoid stale copies fighting each other

## Build

Root CMake requires:

- `-DDEVICE=<device>`
- `-DMODE=<DEVELOPMENT|RELEASE|RELEASECANDIDATE|BETA|NIGHTLY|UNDEFINED>`

Optional:

- `-DFAMILY=<family>`

### ESP-IDF

- Load `ESP-IDF v5.3.4` into enviorment
- If the user provides an ESP-IDF path, use it
- Otherwise search common local install locations for a `v5.3.4` install
- Do not patch or edit files under the ESP-IDF install directory

### Build Guidance

- Configure from the repo root
- Use the normal CMake/Ninja workflow
- Use `-DFAMILY` only when a non-default family override is actually required

## Device Layer

`Devices/Device.h` is the hardware contract expected by the OS.

If adding a hardware capability:

1. extend the device layer if the concept is truly hardware-facing
2. expose an OS wrapper in `OS/` only if higher layers should consume it

Do not leak board-specific details into application code without a strong reason.

## UI

MatrixOS has a UI/component system under `OS/UI`.

If changing UI behavior:

- inspect existing `OS/UI` patterns first
- preserve current interaction patterns unless intentionally redesigning
- avoid global side effects without checking the active app and current UI stack

## Concurrency and Memory

This codebase mixes STL containers, FreeRTOS, and manual allocation.

Be careful about:

- `pvPortMalloc()` / `vPortFree()`
- task stacks and queue sizes
- dynamic allocation in hot paths
- using `reserve()` when indexed writes actually require `resize()`

Rules of thumb:

- if you index a `vector`, it must already be sized
- free RTOS heap allocations with the matching RTOS free
- bounds-check protocol-derived indexes and offsets
- do not assume heap fragmentation behaves well on embedded targets

## Debugging

- Use the repo logging macros instead of ad hoc prints
- log state transitions, failing sizes, indexes, and error codes
- keep logs concise enough for serial output

## Workflow

Recommended workflow:

1. identify the correct layer
2. read the public API and concrete implementation
3. search for an existing pattern before inventing a new one
4. make the smallest coherent change
5. preserve persistence and compatibility rules intentionally
6. keep agent-only workflow files under `.agent/`

Agent-only notes, journals, prompt files, scratch docs, and local helper scripts belong in `.agent/`, not the repo root or official `Tools/`.

## Style

### Strong Rules

- use `nullptr`, not `NULL`
- use `Function()`, not `Function(void)`, unless an external interface requires it
- use PascalCase for types and methods
- use lowerCamel for variables, parameters, fields, and booleans
- use `Type* name` and `Type& name`
- avoid `_` in C++ names unless required by macros, external interfaces, or compatibility surfaces
- user-visible strings should say `map`; internal protocol names may still say `UAD`
- non-trivial modules should define and use a stable local `TAG`

### Naming

- types, enums, namespaces, and methods: PascalCase
- locals, parameters, fields: lowerCamel
- booleans: `isX`, `hasX`, `canX`, or a clear adjective
- macros and shared compile-time constants: `ALL_CAPS`

### Layout

- use 2 spaces in C++ files
- do not introduce tabs
- function braces on the same line
- control-flow braces on the next line
- namespace braces on the next line

### Pointers and Includes

- use `Type* name` and `Type& name`
- prefer `const vector<string>& args`
- include order: local project headers, related subsystem headers, then standard library headers
- use `#pragma once` for headers

### Comments and Logging

- keep comments short and factual
- explain why, not the obvious what
- use `TODO:` only for real follow-up work
- keep log tags stable and short
- prefer one informative log over many low-signal logs

### Safety

- check bounds before indexing with protocol-derived values
- check allocation and I/O return values
- prefer explicit fallback behavior over undefined behavior
- use `resize()` before indexed writes

### Generated Files

- do not hand-edit generated files
- edit the source CMake, headers, or generator inputs instead

## Reading Order

Start with the public OS API, the device contract, the application base class, and then the subsystem you actually need to change.
