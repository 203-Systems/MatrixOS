# Input System Revision 2

## Purpose
This document records the second round of revisions to the current input-system plan.

This revision focuses on four things:

- whether `memberId` is dense or fully opaque
- what kinds of clusters the current `Point`-based APIs actually apply to
- whether `TouchArea` fits the current `InputClusterShape`
- where cluster mapping handlers should live

## Conclusions

- `memberId` should be **stable and dense within a cluster**
- `GetInputAt / GetInputsAt / TryGetPoint` should only apply to **discrete coordinate-addressable clusters**
- `TouchArea` should not be forced into the same shape semantics as `Grid2D`
- mapping handlers should be stored **inside `InputCluster`**, not in a parallel ops table

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

## Direct Revisions To Revision 1

After this revision, the following extra rules are added on top of Revision 1:

1. `memberId` must be dense within `[0, inputCount)`
2. `memberId` remains device-defined and stable, but OS still must not derive layout from it
3. `GetInputAt / GetInputsAt / TryGetPoint` are only for discrete coordinate-addressable clusters
4. `TouchArea` should use a continuous 2D shape such as `Area2D`, not `Grid2D`
5. mapping handlers should live directly inside `InputCluster`

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
- fold Revision 1 and Revision 2 conclusions back into the main `InputSystem.md`

## One-Sentence Summary

The core of Revision 2 is:

**keep `memberId` dense but opaque, restrict integer `Point` APIs to discrete clusters, and bind mapping handlers directly to each `InputCluster`.**
