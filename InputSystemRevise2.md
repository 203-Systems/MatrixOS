# Input System Revision 2

## Purpose
This document records the second round of revisions to the current input-system plan.

This revision focuses on four things:

- whether `memberId` is dense or fully opaque
- what kinds of clusters the current `Point`-based APIs actually apply to
- whether `TouchArea` fits the current `InputClusterShape`
- where cluster mapping handlers should live
- how temporary the old `KeyPad` compatibility layer should be
- whether `MatrixOS::SYS::Rotate()` should remain as a public wrapper
- whether `MatrixOS::Input` is still too keypad-centric
- whether rotation metadata should remain in `InputCluster`

## Conclusions

- `memberId` should be **stable and dense within a cluster**
- `GetInputAt / GetInputsAt / TryGetPoint` should only apply to **discrete coordinate-addressable clusters**
- `TouchArea` should not be forced into the same shape semantics as `Grid2D`
- mapping handlers should be stored **inside `InputCluster`**, not in a parallel ops table
- the old `KeyPad` layer should only survive as a **short-lived migration shim**
- `MatrixOS::SYS::Rotate()` may temporarily forward to `Device::Rotate()`, but should not remain a long-term public API
- `MatrixOS::Input` core should not remain centered around keypad/grid helpers
- once mapping is fully device-owned, `InputCluster` should not keep rotation implementation details

## 1. `memberId` Must Be Stable And Dense

Revision 1 correctly redefined `memberId` as a device-defined identity rather than an OS-derived layout index.

However, there is still one missing constraint:

- should `memberId` be completely opaque and sparse
- or should it be stable but still dense within `[0, inputCount)`

This revision locks that down:

- `memberId` is still device-defined
- `memberId` is still not interpreted by OS using formulas
- but `memberId` **must** remain within `[0, inputCount)`
- and it **must** remain stable for the lifetime of that member

This gives the system the best balance:

- device layer still owns the meaning
- OS can still allocate per-cluster state arrays safely
- touch slots can use stable `memberId`s
- event queues and state caches stay simple

### What This Means

For example:

- a main 8x8 grid can use `memberId = 0..63`
- a 10-slot `TouchArea` can use `memberId = 0..9`
- a fader cluster with 4 channels can use `memberId = 0..3`

But OS must **not** assume any formula such as:

```cpp
x = memberId % width;
y = memberId / width;
```

The only valid generic assumptions are:

- `0 <= memberId < inputCount`
- the same physical/logical member keeps the same `memberId`

## 2. `Point`-Based APIs Only Apply To Discrete Coordinate-Addressable Clusters

The current API set:

```cpp
void GetInputsAt(Point xy, vector<InputId>* ids);
bool GetInputAt(uint8_t clusterId, Point xy, InputId* id);
bool TryGetPoint(InputId id, Point* xy);
```

is useful, but only for one specific category of clusters:

- clusters that are discrete
- clusters that are addressable by integer coordinates

This revision makes that explicit.

These APIs are appropriate for:

- `Keypad`
- discrete `Linear1D`
- any future discrete grid or discrete strip

These APIs are **not** the right abstraction for:

- `TouchArea`
- continuous surfaces
- multi-touch spatial data
- any cluster whose natural coordinate is not integer `Point`

### Rule

`GetInputAt / GetInputsAt / TryGetPoint` should be documented and implemented as APIs for **discrete coordinate-addressable clusters only**.

They should not pretend to be universal for every `InputClass`.

## 3. `TouchArea` Needs A Different Shape Semantics

The current shape enum is:

```cpp
enum class InputClusterShape : uint8_t {
  Scalar = 0,
  Linear1D,
  Grid2D,
};
```

This works for discrete clusters, but `TouchArea` is different:

- it is continuous
- it may support multi-touch
- it may report `PointFloat`
- it should not imply integer `Point <-> memberId` mapping

Therefore, `TouchArea` should not be treated as just another `Grid2D`.

### Recommended Direction

Add one more shape for continuous 2D input, for example:

```cpp
enum class InputClusterShape : uint8_t {
  Scalar = 0,
  Linear1D,
  Grid2D,
  Area2D,
};
```

Meaning:

- `Grid2D` means discrete 2D addressable members
- `Area2D` means continuous 2D spatial input

This keeps the semantics clean:

- `Keypad` belongs to `Grid2D`
- `TouchArea` belongs to `Area2D`

`TouchArea` may still use dense `memberId`s for touch slots, but that does **not** mean it participates in the same integer coordinate APIs as a keypad.

## 4. Mapping Handlers Should Live Inside `InputCluster`

Revision 1 already rejected the idea of a separate active registration model like `RegisterInputClusters()`.

One remaining detail still needed clarification:

- should mapping handlers live in a separate `clusterOps` table
- or should they live directly inside `InputCluster`

This revision chooses the second option.

### Why

A parallel table such as:

```cpp
extern vector<InputCluster> clusters;
extern vector<ClusterMappingOps> clusterOps;
```

creates an unnecessary sync problem:

- both tables must always stay aligned
- rotation rebuilds can reorder one without the other
- missing ops entries become ambiguous
- runtime bugs appear as cross-cluster mapping mistakes

The simpler and safer approach is:

```cpp
struct InputCluster {
  uint8_t clusterId;
  string name;
  InputClass inputClass;
  InputClusterShape shape;
  Point rootPoint;
  Dimension dimension;
  uint16_t inputCount;

  bool (*TryGetPoint)(uint16_t memberId, Point* point) = nullptr;
  bool (*TryGetMemberId)(Point point, uint16_t* memberId) = nullptr;
};
```

### Rule

- cluster descriptive data and discrete mapping entry points belong together
- device layer owns and populates the full `InputCluster`
- OS reads and dispatches only
- no parallel ops table

### Important Limitation

These handlers are still only appropriate for discrete coordinate-addressable clusters.

For `TouchArea` / `Area2D`:

- these function pointers may remain `nullptr`
- continuous-space APIs can be added later if needed

## 5. The Old `KeyPad` Layer Must Not Survive As A Second Real Input System

The current branch still uses the old device-side path:

- device driver produces `KeyEvent / KeyInfo`
- `MatrixOS::KeyPad` receives it
- `MatrixOS::KeyPad` bridges it into `InputEvent`

That is acceptable only as a migration step.

It must **not** become the long-term shape of the new system.

### Rule

The compatibility layer may temporarily exist, but it must be reduced to a thin shim only:

- no independent mapping rules
- no independent rotation logic
- no second state cache
- no second source of truth

Long term, device drivers should produce `InputEvent` directly.

If the old `KeyPad` layer remains, it should only forward into the new system, never reinterpret the device topology on its own.

## 6. `MatrixOS::SYS::Rotate()` Should Be Treated As Transitional, Not Final

Revision 1 already decided that rotation ownership belongs to the device layer.

That means the following is acceptable only as a migration convenience:

```cpp
MatrixOS::SYS::Rotate(...)
  -> Device::Rotate(...)
```

This wrapper does not satisfy the architectural goal by itself.

### Rule

- the authoritative rotation entry point is `Device::Rotate(...)`
- `MatrixOS::SYS::Rotate()` may temporarily forward to it
- but it should not remain a permanent public API

Otherwise, new code will continue to treat rotation as an OS-owned transaction.

## 7. `MatrixOS::Input` Core Must Not Stay Keypad-Centric

The current `MatrixOS::Input` layer still contains helpers such as:

- `GetPrimaryGridCluster()`
- `GetPrimaryGridSize()`
- `GetKeypadState(Point)`
- `HasVelocitySensitivity()`

These can be useful during migration, but they do not belong to the conceptual core of a generic input system.

### Rule

- these APIs should be treated as transition helpers
- they should not define the long-term identity of `MatrixOS::Input`
- the conceptual core should remain:
  - event queue
  - state cache
  - cluster enumeration
  - device-owned mapping dispatch

The generic input system should not slowly turn into "KeyPad with a new name."

## 8. `InputCluster` Should Not Permanently Keep Rotation Implementation Fields

If the final architecture is:

- device layer owns rotation
- device layer owns mapping
- device layer produces final post-rotation coordinates

then fields such as:

- `rotation`
- `rotationDimension`

inside `InputCluster` are likely temporary implementation details rather than proper long-term cluster metadata.

### Rule

- `InputCluster` should eventually keep descriptive data plus mapping entry points
- rotation bookkeeping that only exists to help the current implementation should move behind the device-side mapping handlers

Keeping these fields forever would leak a specific mapping strategy back into the shared framework type.

## 9. Mystrix1 Bring-Up Shows That Event Production Alone Is Not Enough

Current Mystrix1 behavior already demonstrates an important architectural rule:

- the legacy keypad driver still produces events
- those events are successfully bridged into `InputEvent`
- but pad presses still do not reach UI components if device-side cluster mapping is missing

This happens because:

- function-key handling can bypass coordinate lookup
- normal pad events still require `InputId -> Point` mapping before UI dispatch can happen
- if `Device::Input::TryGetPoint(...)` is missing, grid events become invisible to the new UI path

### Rule

For devices that still use the old keypad driver during migration:

- event bridging alone is not sufficient
- the device must also provide at least a minimal discrete cluster model and working coordinate mapping

That means Mystrix1 bring-up requires, at minimum:

- a function-key cluster
- a main grid cluster
- any required touchbar clusters
- working `TryGetPoint`
- working `TryGetMemberId`

Without those pieces, the system will appear half-alive:

- function key works
- normal pad interaction does not

This is not just an implementation bug. It is a structural reminder that the new input system depends on both:

- event production
- coordinate resolution

## Direct Revisions To Revision 1

After this revision, the following extra rules are added on top of Revision 1:

1. `memberId` must be dense within `[0, inputCount)`
2. `memberId` remains device-defined and stable, but OS still must not derive layout from it
3. `GetInputAt / GetInputsAt / TryGetPoint` are only for discrete coordinate-addressable clusters
4. `TouchArea` should use a continuous 2D shape such as `Area2D`, not `Grid2D`
5. mapping handlers should live directly inside `InputCluster`
6. the old `KeyPad` layer is only a temporary migration shim, not a second long-term input owner
7. `MatrixOS::SYS::Rotate()` is transitional if it remains, while `Device::Rotate()` is authoritative
8. keypad-centric helpers in `MatrixOS::Input` are migration helpers, not core semantics
9. `InputCluster` should eventually drop rotation implementation fields
10. migration-era devices must provide both event bridging and minimal cluster/mapping support, otherwise only special non-spatial inputs will work

## Required Next Implementation Steps

### Priority 1
- rename `localIndex` to `memberId`
- document that `memberId` is dense within `[0, inputCount)`
- keep `memberId` device-defined and stable

### Priority 2
- narrow `GetInputAt / GetInputsAt / TryGetPoint` semantics to discrete coordinate-addressable clusters
- make sure `TouchArea` does not rely on them

### Priority 3
- add a continuous 2D shape such as `Area2D`
- assign `TouchArea` to that shape

### Priority 4
- merge discrete mapping handlers into `InputCluster`
- remove any idea of a parallel `clusterOps` table

### Priority 5
- reduce `MatrixOS::KeyPad` to a thin temporary shim
- stop adding new logic there

### Priority 6
- keep `Device::Rotate(...)` as the authoritative rotation entry
- plan removal of `MatrixOS::SYS::Rotate()` from the long-term public API

### Priority 7
- classify keypad/grid convenience APIs as migration helpers
- keep them out of the long-term core abstraction

### Priority 8
- move rotation bookkeeping behind device-side mapping handlers
- eventually remove `rotation` and `rotationDimension` from `InputCluster`

### Priority 9
- for Mystrix1 and other migration-era devices, add the minimal cluster + mapping layer before expecting UI parity

### Priority 10
- fold Revision 1 and Revision 2 conclusions back into the main `InputSystem.md`

## One-Sentence Summary

The core of Revision 2 is:

**keep `memberId` dense but opaque, restrict integer `Point` APIs to discrete clusters, bind mapping handlers directly to each `InputCluster`, and avoid letting transitional keypad-era APIs define the long-term architecture.**
