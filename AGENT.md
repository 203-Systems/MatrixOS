# MatrixOS Agent Guide

This document is for future AI agents and engineers modifying MatrixOS.

Its goal is to explain:

- how the repository is organized
- which layer to edit for a given change
- which APIs are intended to be used by applications
- how persistence, storage, HID, MIDI, UI, and applications fit together
- how to keep changes stylistically consistent with the existing codebase

If this guide and the code disagree, trust the code first.

## External References

Primary external documentation:

- Matrix Wiki and Developer API: <https://matrix.203.io/>

This site is the public API / SDK / documentation entry point. The homepage exposes a "Developer API" path and versioned Matrix OS docs.

Recommended local companion repo:

```powershell
git clone https://github.com/203-Systems/Matrix-Wiki ..\Matrix-Wiki
```

Use the local wiki clone when:

- you need broader architecture and user-facing docs
- you are changing APIs and want to update docs in the same pass
- you want searchable local references while working offline

## Mental Model

MatrixOS is split into three main layers:

1. `Devices/`
   This is the hardware layer. It owns board-specific drivers, NVS, storage, keypad scanning, LEDs, USB details, and chip-family integration.

2. `OS/`
   This is the OS/runtime layer. It wraps device functionality behind the `MatrixOS::` API, owns app lifecycle, UI runtime, HID/MIDI subsystems, logging, storage wrappers, and shared framework code.

3. `Applications/`
   This is the app layer. Apps inherit from `Application`, use `MatrixOS::` APIs, and are launched by the supervisor as the active app task.

When deciding where to modify something:

- hardware behavior, board capabilities, storage drivers, USB descriptors at chip level: `Devices/`
- public OS module behavior or shared subsystem logic: `OS/`
- product feature behavior inside a specific app: `Applications/`

## Key Entry Points

Start here before making structural changes:

- `OS/MatrixOS.h`
  Public OS API exposed to applications and many system components.

- `Applications/Application.h`
  Base class for apps and app registration helpers.

- `OS/System/System.cpp`
  App factory, supervisor, OS init order, and system module initialization.

- `Devices/Device.h`
  Required hardware-facing interface for a device family / variant.

- `Applications/CMakeLists.txt`
  App loading, registration, generated `Applications.h`, and family-provided app lists.

- `OS/System/Parameters.h`
  Global stack, queue, version, and timing constants.

## Boot and Runtime Flow

High-level boot path:

1. `MatrixOS::SYS::Begin()` initializes the device.
2. MIDI is initialized before USB so the OS MIDI port exists early.
3. USB and system modules are initialized.
4. system NVS is updated.
5. a default boot animation app is scheduled.
6. the device starts.
7. the supervisor task starts and owns app lifecycle.

Important consequences:

- only one app is active at a time
- apps run in a dedicated FreeRTOS task created by the supervisor
- app exit is handled by `MatrixOS::SYS::ExitAPP()`
- app memory is allocated with `pvPortMalloc()` and destroyed manually

Relevant files:

- `OS/System/System.cpp`
- `Applications/Application.h`

## Application Model

Every app derives from `Application` and overrides:

- `Setup(const vector<string>& args)`
- `Loop()`
- `End()`

`Application::Start()` calls `Setup()` once, then repeatedly calls `Loop()` forever until the app exits.

To exit an app:

```cpp
MatrixOS::SYS::ExitAPP();
```

Do not manually kill the app task from application code unless you are intentionally changing lifecycle internals.

## App Registration

Apps are registered through CMake and generated headers, not by manually editing a central source file.

Important details:

- `Applications/CMakeLists.txt` tracks registered apps
- it generates `Applications.h` into the build directory
- the generated header uses `REGISTER_APPLICATION(...)`

Implications:

- do not hand-edit generated `Applications.h`
- if adding a new app, update its app-level CMake and ensure it is included by the family app list
- check the current device family `ApplicationList.txt` to see what is actually built

## Public OS APIs Applications Should Prefer

The main app-facing API lives in `OS/MatrixOS.h`.

Common namespaces:

- `MatrixOS::SYS`
  Time, delays, reboot, bootloader, app launch/exit, settings, error handler.

- `MatrixOS::LED`
  LED layer creation, fill, fade, per-key color, partition fill, brightness control.

- `MatrixOS::KeyPad`
  key event retrieval, key lookup by XY or ID, queue clearing.

- `MatrixOS::USB::CDC`
  debug or serial-style host communication.

- `MatrixOS::MIDI`
  MIDI send/get and SysEx send helpers.

- `MatrixOS::HID`
  keyboard, gamepad, raw HID.

- `MatrixOS::NVS`
  small persistent blobs.

- `MatrixOS::FileSystem`
  app-sandboxed file storage when device storage is available.

Prefer these wrappers over calling `Device::...` directly from application code.

Direct `Device::...` use is usually only appropriate for:

- device layer code
- OS internals
- carefully justified performance-sensitive or capability-specific work

## Storage and Persistence

There are two persistence mechanisms:

1. NVS
   Small blobs, key-value style, backed by device NVS.

2. FileSystem
   SD / storage-backed filesystem if `DEVICE_STORAGE == 1` and storage is available.

File system notes:

- availability is gated by `MatrixOS::FileSystem::Available()`
- app paths are sandboxed under:

```text
/MatrixOS/AppData/<author>-<app-name>/
```

- use `MatrixOS::FileSystem::Open`, `Exists`, `MakeDir`, `Remove`, `Rename`, `ListDir`

If you are designing persistence for a feature:

- use NVS for small settings or compact state
- use FileSystem for larger structured data, user content, history, or anything likely to outgrow NVS
- if a protocol or app can store to either backend, define a clear precedence and cleanup strategy so stale copies do not fight each other

## Build System

The root CMake requires:

- `-DDEVICE=<device>`
- `-DMODE=<DEVELOPMENT|RELEASE|RELEASECANDIDATE|BETA|NIGHTLY|UNDEFINED>`

Optional:

- `-DFAMILY=<family>`

Important build facts:

- the selected family contributes `Config.cmake`
- family config defines `DEVICE_STORAGE` and `DEVICE_BATTERY`
- application loading depends on the family `ApplicationList.txt`
- some applications may be cloned from external git repos at configure time

### ESP-IDF Requirement

- use `ESP-IDF v5.3.4`
- if the user has already provided an ESP-IDF path, use that path
- if the user has not provided a path, search common local install locations and identify the `v5.3.4` install before building
- do not patch or edit files under the ESP-IDF install directory; fix build issues in MatrixOS instead

### Standard Build Commands

Typical configure/build flow from the repo root:

```powershell
cmake -S . -B build\Mystrix1 -G Ninja -DDEVICE=Mystrix1 -DMODE=DEVELOPMENT
cmake --build build\Mystrix1
```

```powershell
cmake -S . -B build\Mystrix2 -G Ninja -DDEVICE=Mystrix2 -DMODE=DEVELOPMENT
cmake --build build\Mystrix2
```

Use `-DFAMILY=...` only when the build actually needs a non-default family override.

If a feature only exists on devices with storage or battery, guard it using the existing compile-time feature flags and runtime availability checks.

## Device Layer Expectations

`Devices/Device.h` defines the contract the OS expects from hardware.

Examples:

- `Device::LED`
- `Device::KeyPad`
- `Device::NVS`
- `Device::Storage`

If you are adding hardware capability:

- first extend the device layer interface if the concept is truly hardware-facing
- then expose an OS wrapper in `OS/` if apps or higher-level systems should consume it

Do not leak chip- or board-specific implementation details into application code unless there is no practical abstraction boundary.

## UI Model

MatrixOS has a UI/component system in `OS/UI`.

Typical patterns:

- apps create a `UI`
- apps add components
- components react to key events and render via LED buffers
- UI lifecycle is tied to app lifecycle

If changing UI behavior:

- inspect `OS/UI/`
- preserve existing interaction patterns unless intentionally redesigning
- avoid global UI side effects without checking the active app and current UI stack

## Concurrency and Memory

This codebase mixes STL containers with FreeRTOS and manual allocation.

Be careful about:

- `pvPortMalloc()` / `vPortFree()`
- app factory allocation and placement new
- FreeRTOS task stack size
- queue sizes from `OS/System/Parameters.h`
- dynamic allocation in hot paths
- using `reserve()` when you actually need `resize()`

Rules of thumb:

- if you index into a `vector`, it must already be sized
- if a pointer came from `pvPortMalloc()`, free it with `vPortFree()`
- prefer explicit capacity and bounds checks around protocol-derived indexes and offsets
- do not assume heap fragmentation behaves well on embedded targets

## Debugging Strategy

Use logging macros from `OS/MatrixOS.h`:

- `MLOGE`
- `MLOGW`
- `MLOGI`
- `MLOGD`
- `MLOGV`

Prefer these over ad hoc prints inside firmware code.

When debugging embedded failures:

- log both requested and allowed sizes for storage / upload failures
- log protocol command and error code pairs
- log before and after state transitions
- keep logs concise enough to read on serial output

## How to Approach a Change

Recommended workflow for an AI agent:

1. Identify the layer first.
   Is this a device problem, OS subsystem problem, or app problem?

2. Read the public API and the concrete implementation.
   Start with `OS/MatrixOS.h`, then the actual module `.cpp` / `.h`.

3. Search for similar patterns already in repo.
   Reuse local conventions before inventing a new pattern.

4. Make the smallest coherent change.
   Avoid broad refactors unless the task actually requires them.

5. Preserve persistence and compatibility rules intentionally.
   Do not create accidental dual sources of truth.

6. Keep agent-only workflow artifacts out of the main project surface.
   Local agent notes, Copilot journals, temporary prompt files, wrapper scripts,
   migration scratch docs, and similar AI-workflow-only materials should live
   under `.agent/`, not in the repo root and not in official project `Tools/`
   locations.


## Formatting Guide

Use these rules consistently.

### Strong Rules

- use `nullptr`, not `NULL`
- use `()`, not `(void)`, except for C callbacks or external interfaces that require it
- use PascalCase for type names; do not create underscored type names like `Application_Info`
- use `Type* name` and `Type& name`
- user-visible strings should say `map`; internal protocol names may still say `UAD`
- non-trivial modules should define and use a stable local `TAG`
- avoid `_` in C++ names unless the symbol is `ALL_CAPS`, a required external name, or an existing compatibility surface
- use PascalCase for types and methods
- use lowerCamel for variables, parameters, fields, and booleans

### C++ Naming

#### Namespaces

Use PascalCase or established acronym casing:

- `MatrixOS`
- `Device`
- `SYS`
- `LED`
- `MIDI`
- `HID`

Do not rename existing namespaces for style reasons.

#### Classes, structs, and type names

Use PascalCase:

- `Application`
- `CustomControlMap`
- `TaskPermissions`
- `StorageStatus`

Examples:

- `Application`
- `CustomControlMap`
- `TaskPermissions`
- `StorageStatus`

#### Functions and methods

The dominant style is PascalCase / UpperCamelCase:

- `LoadUAD`
- `SendSysEx`
- `GetTopLayer`
- `ExecuteAPP`
- `SetBrightness`

This also applies to boolean-returning functions:

- `Available`
- `Connected`
- `Loaded`

Do not use `snake_case` for C++ functions unless you are implementing a required C callback or external interface.

#### Enum types and enum values

Enum type names should be PascalCase:

- `MidiType`
- `ActionEventType`
- `LayerActionMode`

Use PascalCase for scoped enum values.

Use `ALL_CAPS` only for protocol constants, command IDs, and existing macro-style enums.

#### Macros and compile-time constants

Use all caps for macros and macro-like constants:

- `MLOGD`
- `REGISTER_APPLICATION`
- `MATRIXOS_MAJOR_VER`
- `KEYEVENT_QUEUE_SIZE`
- `MAX_UAD_LAYER`

Rule:

- preprocessor macros: `ALL_CAPS`
- shared compile-time constants: `ALL_CAPS`
- local `const` variables inside a function: `lowerCamel`

#### Variables and fields

Examples:

- `selectedLayer`
- `deviceDesc`
- `trackSelectorBtn`
- `tickTaskHandle`
- `currentView`

Rule:

- locals: `lowerCamel`
- parameters: `lowerCamel`
- fields: `lowerCamel`
- do not introduce `snake_case` names into C++ unless required by an external interface

#### Boolean naming

Examples:

- `isSystem`
- `hasDefault`

Rule:

- use `isX`, `hasX`, `canX`, or a clear adjective
- do not use underscored boolean names like `is_system`

### C++ Layout and Spacing

#### Indentation

- use 2 spaces in C++ files
- do not introduce tabs

Some older files visually align wrapped lines more aggressively. Match the local wrapping style where practical.

#### Braces

The dominant style is:

- function definitions: opening brace on the same line
- control flow blocks: opening brace on the next line
- namespaces: opening brace on the next line

Examples:

```cpp
void CustomControlMap::Loop() {
  ...
}

if (uadData == nullptr)
{
  return;
}

namespace MatrixOS::SYS
{
  ...
}
```

Use this consistently.

#### Pointer and reference spacing

Pointer style is mostly:

- `Type* name`

Examples:

- `KeyInfo* keyInfo`
- `Application* app`
- `uint8_t* uadData`

Rule:

- for pointers, use `Type* name`
- for references, use `Type& name`
- prefer `const vector<string>& args` over `const vector<string> &args`

#### Null and empty parameter style

Use:

- `nullptr` in new C++ code
- `Function()` in new C++ declarations and definitions

Keep `NULL` and `(void)` only in C files, third-party code, or required external callback signatures.

#### Includes

Typical include order:

1. local project headers
2. related subsystem headers
3. standard library headers

Use blank lines to separate meaningful groups if the file already does so.

Use `#pragma once` for headers.

### Comments and Documentation

- keep comments short and factual
- explain why, not the obvious "what"
- use sentence fragments unless a full sentence is clearer
- avoid decorative comments
- use `TODO:` only for real unresolved follow-up work
- if a data format is non-obvious, document the schema near the parser/serializer

Good examples:

- noting a wire-format placeholder
- explaining a fallback order
- explaining why a buffer must be resized instead of reserved

Bad examples:

- repeating the next line in prose
- narrating trivial assignments

### Logging Style

Use:

- `MLOGE`
- `MLOGW`
- `MLOGI`
- `MLOGD`
- `MLOGV`

Guidelines:

- keep tags stable and short
- in non-trivial modules, define one local `TAG` and reuse it consistently
- include the failing size, index, offset, command, or error code when that explains the failure
- do not spam logs in tight loops unless the loop is itself the bug under investigation
- prefer one informative log over many low-signal logs

### Control Flow and Safety

- check bounds before indexing vectors with protocol-derived values
- check allocation and I/O return values
- prefer explicit fallback behavior over implicit undefined behavior
- use `resize()` before indexed writes into vectors
- keep persistence precedence explicit when multiple storage backends exist

### Generated Files

- do not hand-edit generated files such as the build-generated `Applications.h`
- edit the source CMake or source headers that generate them

## Practical Tips for This Repo

- Search first with `rg`
- Read before refactoring
- Treat `OS/MatrixOS.h` as the app-facing contract
- Treat `Devices/` as hardware-specific implementation
- Treat `Applications/CustomControlMap` as protocol-sensitive and editor-coupled
- Treat `Matrix-Wiki` and `matrix.203.io` as supporting references, not replacements for code

## Suggested First Reading Order

For a new agent, this is a good order:

1. `README.md`
2. `OS/MatrixOS.h`
3. `Applications/Application.h`
4. `OS/System/System.cpp`
5. `Devices/Device.h`
6. `OS/System/Parameters.h`
7. the target subsystem you actually need to change

For docs:

1. <https://matrix.203.io/>
2. local clone of `Matrix-Wiki`
