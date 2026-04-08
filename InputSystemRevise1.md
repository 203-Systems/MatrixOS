# Input System Revision 1

## Purpose
This document records the first round of revisions to the current `InputSystem` plan.

This revision focuses on two things only:

- rotation ownership boundaries
- cluster coordinate mapping ownership boundaries

The conclusion is straightforward:

- `Rotation` should no longer be owned by `OS`
- `memberId <-> XY` conversion should not be hardcoded inside `OS/Input` using generic formulas

## Current Problems

Although the current branch has already added `rotation` fields and cluster metadata into `InputCluster`, the actual path is still:

- `MatrixOS::SYS::Rotate()` updates the global rotation
- `OS/Input` receives the notification
- `OS/Input` rotates `Point` on its own
- `OS/Input` converts `memberId -> x/y -> Point` on its own
- `OS/Input` converts `Point -> memberId` on its own

This creates several problems:

1. Wrong ownership boundary
   - Rotation is fundamentally a device layout concern, not a generic OS behavior

2. `memberId` is being treated as a regular layout index by OS
   - It currently assumes `Grid2D => y * width + x`
   - It currently assumes `Linear1D => x` or `y`
   - This only works for the simplest regular matrix layouts

3. It does not fit future input types
   - irregular physical layouts
   - sparse matrices with holes
   - TouchArea
   - multi-touch slots
   - MPE
   - any future cluster that is not a regular row-major layout

4. It can easily double-rotate together with the old `KeyPad` path
   - the new Input path rotates once
   - the old `KeyPad::XY2ID/ID2XY` path rotates again

5. `FunctionKey` should not be hardcoded inside `InputId`
   - `InputId{0, 0}` is a device-specific implementation detail of some current devices
   - this is not a framework-level generic semantic
   - hardcoded helpers such as `InputId::FunctionKey()` and `InputId::IsFunctionKey()` push device topology back into OS

6. `scanRateHz` and `pressureMax` should not be part of generic capabilities
   - they are not generic input capabilities that upper layers should depend on
   - `scanRateHz` is an internal device implementation detail, usually decided by device-side tasks or scan loops
   - `pressureMax` is also an internal device range detail and should not be required knowledge for applications
   - if this information is ever needed, it should be exposed through device-specific diagnostics or debug APIs, not generic input capabilities

7. An active registration model like `RegisterInputClusters()` is not a great fit
   - it does not match the current device-owned data style used by `LEDPartition`
   - it introduces unnecessary complexity around init order, duplicate registration, stale state, and re-registration after rotation
   - cluster data is better held directly by the device layer, with OS only reading it

## Revised Core Principles

### 1. Remove `MatrixOS::SYS::Rotate()`
`Rotation` should no longer exist as a public OS-owned transaction.

This API should be removed:

```cpp
MatrixOS::SYS::Rotate(Direction rotation, bool absolute = false);
```

It should be replaced by a device-layer entry point:

```cpp
namespace Device
{
void Rotate(Direction rotation, bool absolute = false);
}
```

If the system layer needs to trigger a rotation, it should directly call `Device::Rotate(...)`.

### 1.1 Move `FunctionKey` definition into the device layer
`FunctionKey` is not part of the `InputId` type itself. It is a device-defined special input.

Therefore:

- `InputId` should no longer keep hardcoded helpers such as `FunctionKey()`
- `InputId` should no longer keep hardcoded checks such as `IsFunctionKey()`

The device layer should define it instead:

```cpp
namespace Device
{
InputId GetFunctionKeyId();
bool IsFunctionKey(InputId id);
}
```

If upper layers need a stable access point, `MatrixOS::Input` may provide a forwarding wrapper:

```cpp
namespace MatrixOS::Input
{
InputId GetFunctionKeyId();
bool IsFunctionKey(InputId id);
}
```

But the authoritative definition must come from the device layer.

### 1.2 Narrow generic capabilities
Generic input capabilities should only expose behavior differences that upper layers truly need to know about.

Therefore:

- `scanRateHz` should not remain in `KeypadCapabilities`
- `pressureMax` should not remain in `KeypadCapabilities`

A better direction is:

```cpp
struct KeypadCapabilities {
  bool hasPressure;
  bool hasAftertouch;
  bool hasVelocity;
  bool hasPosition;
};
```

If scan rate, physical ranges, or filtering parameters are ever needed, they should be exposed through device diagnostics APIs rather than the generic input abstraction.

### 1.3 Rename `InputId`
The name `localIndex` is misleading because it suggests:

- a sequential array index
- an OS-derived index
- something that can always be converted using formulas such as `y * width + x`

At the same time, `localId` is visually awkward in many fonts because lowercase `l` and uppercase `I` are hard to distinguish.

This revision recommends renaming the field to:

```cpp
struct InputId {
  uint8_t clusterId;
  uint16_t memberId;
};
```

Meaning:

- `clusterId` identifies which cluster the input belongs to
- `memberId` identifies a device-defined member within that cluster

`memberId` should be treated as an opaque id, not a default array index.

### 1.4 Let the device layer own InputCluster data
`InputCluster` should not continue to use an active registration model such as `RegisterInputClusters()`.

A better fit for this repository is:

- the device layer directly owns the cluster data
- the device layer directly owns the cluster mapping handlers
- OS only reads and dispatches

This is more consistent with the existing `LEDPartition` organization.

Recommended direction:

```cpp
namespace Device::Input
{
enum ClusterId : uint8_t {
  FunctionKey = 0,
  MainGrid = 1,
  TouchBarLeft = 2,
  TouchBarRight = 3,
};

extern vector<InputCluster> clusters;

struct ClusterMappingOps {
  bool (*TryGetPoint)(uint16_t memberId, Point* point);
  bool (*TryGetMemberId)(Point point, uint16_t* memberId);
};

extern vector<ClusterMappingOps> clusterOps;
}
```

Key point:

- cluster data comes from the device layer
- coordinate mapping comes from the device layer
- OS does not register, infer, or interpret layouts

### 1.5 Do not hardcode cluster ids in OS
OS should not define one universal cluster id scheme for all devices.

A better rule is:

- each device or family defines its own cluster id constants
- OS treats `clusterId` as an opaque id
- only special inputs that must be accessed uniformly across devices should be exposed through device-layer APIs

Examples:

- `FunctionKey`
- the primary grid cluster

These should be obtained through device-layer query APIs rather than OS assuming:

- `MainGrid = 1`
- `FunctionKey = {0, 0}`

## 2. Rotation Must Be Fully Owned By The Device Layer
The device layer should own the full rotation transaction, not just store a direction value.

The device layer should be responsible for:

- updating the current device rotation state
- rebuilding input clusters
- rebuilding cluster coordinate mappings
- clearing the input event queue
- clearing the input state cache
- clearing old keypad queues or compatibility caches
- rotating or clearing LED buffers
- resetting touch / MPE / scan-related caches
- handling any other device-specific side effects

The key point is not “the device layer must reimplement every primitive from scratch”, but:

- **the device layer owns the orchestration**
- OS may still provide low-level helpers
- but OS must not initiate and own the rotation transaction

## 3. `memberId` Is Device-Defined, Not OS-Defined
`InputId.memberId` must keep this meaning:

- a device-defined member identifier within a cluster

It must not mean:

- a regular matrix index inferred by OS

That means:

- `memberId -> Point`
- `Point -> memberId`

must both be implemented through **device-assigned handlers**.

OS must no longer hardcode formulas like:

```cpp
localX = memberId % width;
localY = memberId / width;
```

unless a device explicitly chooses that internally.

## 4. OS/Input Should No Longer Own Coordinate Rotation Or Coordinate Conversion
`OS/Input` should be reduced to:

- the input event queue
- the input state cache
- the cluster registry view
- unified dispatch into device-side mapping handlers

`OS/Input` should no longer:

- rotate `Point` based on rotation state by itself
- derive `x/y` from `memberId` by itself
- derive `memberId` from `Point` by itself

## Revised Recommended Model

### InputCluster Should Only Hold Descriptive Data
`InputCluster` should only keep metadata such as:

- `clusterId`
- `name`
- `inputClass`
- `shape`
- `rootPoint`
- `dimension`
- `inputCount`

It should not imply that OS can complete mapping on its own.

### Coordinate Mapping Should Use Handlers
The recommended model is for the device layer to provide a mapping handler for each cluster.

For example:

```cpp
struct InputClusterOps {
  bool (*TryGetPoint)(uint16_t memberId, Point* point);
  bool (*TryGetMemberId)(Point point, uint16_t* memberId);
};
```

Or an equivalent device-layer API:

```cpp
bool Device::Input::TryGetPoint(uint8_t clusterId, uint16_t memberId, Point* point);
bool Device::Input::TryGetMemberId(uint8_t clusterId, Point point, uint16_t* memberId);
```

The specific form is less important than the ownership rule:

- **mapping is decided by the device layer**
- **OS only calls into it**

## Revised API Semantics

### `MatrixOS::Input::TryGetPoint`
Should become:

- look up the cluster
- call the device-layer handler
- do not apply generic coordinate formulas

### `MatrixOS::Input::GetInputAt`
Should become:

- look up the cluster
- call the device-layer `TryGetMemberId`
- do not apply generic coordinate formulas

### `MatrixOS::Input::GetInputsAt`
Should become:

- iterate across all coordinate-aware clusters
- call the device handler for each cluster
- collect all matching `InputId`s

### `MatrixOS::KeyPad::XY2ID / ID2XY`
The compatibility layer should also stop doing its own rotation.

It should instead:

- delegate to the new `MatrixOS::Input` / device mapping
- or call device handlers directly
- and must not add a second independent OS rotation pass

## Buffer Handling Rules

### Input-related Buffers
When rotation changes, the device layer should clear at least:

- the input event queue
- the input state cache
- the old `KeyPad` queue
- old `KeyInfo` states
- `TouchArea` / MPE / multi-touch slot states

Reason:

- after rotation, coordinate semantics have changed
- keeping old state is usually wrong

### LED Buffers
LED buffer policy should be decided by the device layer:

- rotate the existing buffer
- clear the buffer
- or choose a device-specific mixed strategy

But the decision and trigger point must live in the device layer, not in OS.

## Direct Revisions To The Current InputSystem
After this revision, the following ideas in the current plan should be considered outdated:

1. `MatrixOS::SYS::Rotate()` as a long-term public API
2. `OS/Input` handling cluster rotation by itself
3. `OS/Input` deriving `memberId` using `Grid2D/Linear1D` formulas
4. `InputId` hardcoding `FunctionKey`
5. generic `KeypadCapabilities` exposing implementation details such as `scanRateHz` and `pressureMax`
6. using `RegisterInputClusters()` as the long-term cluster lifecycle model

The new hard rules are:

- rotation entry lives in the device layer
- cluster mapping lives in the device layer
- `memberId` is a device-defined opaque identity
- `FunctionKey` is a device-defined special input, not a framework constant
- generic capabilities only expose behavior differences that upper layers truly need
- `InputCluster` data and mapping handlers are owned by the device layer
- OS only dispatches; it does not interpret layout

## Required Next Implementation Steps

### Priority 1
- remove `MatrixOS::SYS::Rotate()`
- add `Device::Rotate(...)`
- move the full rotation transaction into the device layer

### Priority 2
- remove generic `memberId <-> Point` derivation from `OS/Input`
- replace it with a device handler model

### Priority 3
- rename `InputId.localIndex` to `memberId`
- explicitly document that `memberId` is a device-defined opaque id

### Priority 4
- remove hardcoded `FunctionKey` helpers from `InputId`
- replace them with `GetFunctionKeyId / IsFunctionKey` provided by the device layer

### Priority 5
- narrow `KeypadCapabilities` by removing `scanRateHz` and `pressureMax`

### Priority 6
- remove the `RegisterInputClusters()` model
- replace it with a device-owned cluster table and device-owned mapping ops

### Priority 7
- fix the double-rotation bug in the old `KeyPad` compatibility path
- make old compatibility paths use the same device mapping

## One-Sentence Summary
The core of this revision is not “add another rotation callback”, but:

**actually take rotation and mapping out of OS and give them back to the device layer.**
