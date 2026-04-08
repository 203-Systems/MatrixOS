# Generic Input System Design

## Summary
Gradually refactor the current `KeyEvent / KeyInfo / MatrixOS::KeyPad` input stack into a generic input system centered around `InputId / InputEvent / InputSnapshot / InputCluster / MatrixOS::Input`.

At the current stage, only the type layer and design rules under `OS/Framework/Input` are being established. `UI` and `Applications` are not being migrated yet.

## Design Decisions Already Locked In

### Input Identity
Input identity is unified as `InputId`:

```cpp
struct InputId {
  uint8_t clusterId;
  uint16_t localIndex;
};
```

Rules:

- No separate flattened global `id`
- `clusterId + localIndex` is the unique identity of an input
- `InputEvent` and `InputSnapshot` both use the field name `id`

### Events And Snapshots
There is no separate `InputInfo` wrapper layer. Events and snapshots directly carry:

- `id`
- `inputClass`
- `union(XXXInfo)`

Current structure:

```cpp
struct InputEvent {
  InputId id;
  InputClass inputClass;
  union {
    KeypadInfo keypad;
    FaderInfo fader;
    EncoderInfo encoder;
    TouchPointInfo touchPoint;
    GyroInfo gyro;
    AccelerometerInfo accelerometer;
    TemperatureInfo temperature;
    BatteryInfo battery;
    GenericInfo generic;
  };
};

struct InputSnapshot {
  InputId id;
  InputClass inputClass;
  union {
    KeypadInfo keypad;
    FaderInfo fader;
    EncoderInfo encoder;
    TouchPointInfo touchPoint;
    GyroInfo gyro;
    AccelerometerInfo accelerometer;
    TemperatureInfo temperature;
    BatteryInfo battery;
    GenericInfo generic;
  };
};
```

Rules:

- `XXXInfo` uses plain structs
- No inheritance
- No virtual functions
- No `std::variant`
- Every `XXXInfo` is compile-time constrained to `<= 16B`
- Every `XXXInfo` must be trivially copyable
- `InputEvent` and `InputSnapshot` must also be trivially copyable

## InputClass
The currently defined input classes are:

- `Unknown`
- `Keypad`
- `Fader`
- `Encoder`
- `TouchArea`
- `Gyro`
- `Accelerometer`
- `Temperature`
- `Battery`
- `Generic`

Notes:

- `TouchScreen` has been renamed to `TouchArea`
- `TouchStrip` has been removed for now
- `Generic` currently stays intentionally simple and only exposes a `uint64_t value`

## InputCluster
`InputCluster` only describes structure and placement. It does not embed capabilities.

Current fields:

- `clusterId`
- `name`
- `inputClass`
- `shape`
- `rootPoint`
- `dimension`
- `inputCount`

Rules:

- If there is no physical coordinate mapping, `rootPoint = Point::Invalid()`
- Whether a cluster supports coordinate mapping is determined by `shape`, `rootPoint`, and the cluster's own mapping logic
- Do not assume every input has a physical coordinate

### Rotation Ownership
Input rotation handling will move from `OS` down to the device layer.

Rules:

- `OS` no longer owns a single global input rotation transform
- The device layer is responsible for producing the final effective cluster coordinate mapping for the current device orientation
- `InputCluster.rootPoint`
- `InputCluster.dimension`
- `TryGetPoint(localIndex, Point*)`
- `TryGetLocalIndex(Point, uint16_t*)`
  should all reflect the post-rotation result for the current device orientation
- `MatrixOS::Input` and upper layers such as `UI / Applications` should always see the final coordinates that the device layer has already resolved

Why:

- Avoid maintaining a global rotation state inside `OS`
- Avoid device-family-specific branching in `OS`
- Keep input mapping and physical device layout under the same ownership boundary
- If LEDs, inputs, and touch areas all need to follow orientation changes, the device layer is the better place to unify that behavior

## Capabilities
Capabilities are not stored inside `InputCluster`. They are queried through APIs.

Principles:

- Each `InputClass` uses its own strongly typed capabilities structure
- No generic bitmask
- No generic key-value table
- Not every type is required to have a capabilities structure

Example query shape:

```cpp
bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps);
bool GetFaderCapabilities(uint8_t clusterId, FaderCapabilities* caps);
bool GetEncoderCapabilities(uint8_t clusterId, EncoderCapabilities* caps);
```

Call flow:

1. Inspect `inputClass` through `GetCluster(clusterId)`
2. Call the matching class-specific capabilities API

Static capabilities and adjustable configuration remain separate:

- Static capabilities use `GetXxxCapabilities`
- Adjustable configuration uses `GetXxxConfig / SetXxxConfig`

## MatrixOS::Input Public API
The target public API is unified under `MatrixOS::Input`:

```cpp
bool Get(InputEvent* inputEvent, uint32_t timeoutMs = 0);
bool GetState(InputId id, InputSnapshot* snapshot);

const vector<InputCluster>& GetClusters();
const InputCluster* GetCluster(uint8_t clusterId);
const InputCluster* GetPrimaryGridCluster();

Dimension GetPrimaryGridSize();

void GetInputsAt(Point xy, vector<InputId>* ids);
bool GetInputAt(uint8_t clusterId, Point xy, InputId* id);
bool TryGetPoint(InputId id, Point* xy);
```

Semantic rules:

- `GetInputsAt(Point)` uses global coordinates and returns every input hit at that point
- `GetInputAt(clusterId, Point)` also uses global coordinates, but only resolves within the specified cluster
- `TryGetPoint(InputId, Point*)` reverses a single input back into a global coordinate
- `GetPrimaryGridSize()` remains in the first phase as a migration helper for existing grid-based code

## Current Per-Type Info Definitions

### Keypad
- `KeypadState`
  - `Idle`
  - `Activated`
  - `Pressed`
  - `Hold`
  - `Aftertouch`
  - `Released`
  - `Debouncing`
  - `ReleaseDebouncing`
- `KeypadInfo`
  - `lastEventTime`
  - `pressure`
  - `velocity`
  - `state`
  - `hold`
  - `cleared`

### Fader
- `FaderState`
  - `Idle`
  - `Changed`
- `FaderInfo`
  - `lastEventTime`
  - `position`
  - `state`

Notes:

- Fader does not keep `Touched`
- Fader does not keep `Released`

### Encoder
- `EncoderState`
  - `Idle`
  - `Decrement`
  - `Increment`
- `EncoderInfo`
  - `lastEventTime`
  - `state`

Notes:

- `Left / Right` are no longer used
- `Pressed` is not kept

### TouchArea / TouchPoint
`TouchArea` is the input class name, while the concrete payload type is `TouchPointInfo`.

- `TouchPointState`
  - `Idle`
  - `Pressed`
  - `Hold`
  - `Changed`
  - `Released`
- `TouchPointInfo`
  - `lastEventTime`
  - `PointFloat point`
  - `pressure`
  - `state`

Notes:

- Do not use discrete `Point`
- `point` uses `PointFloat`
- `primaryTouch` is not kept

### Gyro
- `GyroState`
  - `Idle`
  - `Changed`
- `GyroInfo`
  - `lastEventTime`
  - `xRate`
  - `yRate`
  - `zRate`
  - `state`

### Accelerometer
- `AccelerometerState`
  - `Idle`
  - `Changed`
- `AccelerometerInfo`
  - `lastEventTime`
  - `xAcceleration`
  - `yAcceleration`
  - `zAcceleration`
  - `state`

### Temperature
- `TemperatureState`
  - `Idle`
  - `Changed`
- `TemperatureInfo`
  - `lastEventTime`
  - `centiDegreesC`
  - `state`

Notes:

- `TemperatureCapabilities` is not kept for now

### Battery
- `BatteryState`
  - `Idle`
  - `Changed`
- `BatteryInfo`
  - `lastEventTime`
  - `milliVolts`
  - `percentage`
  - `state`
  - `charging`
  - `full`

### Generic
- `GenericState`
  - `Idle`
  - `Changed`
- `GenericInfo`
  - `value`
  - `lastEventTime`
  - `state`

Notes:

- `value` is directly a `uint64_t`
- No dynamic layout is used for now

## TouchArea Coordinates And MPE
`TouchPointInfo.point` is defined as:

- A cluster-local continuous coordinate
- Not fixed to the range `0 ~ 1`
- The valid range is determined by the cluster's `dimension`

Example:

- For an 8x8 `TouchArea`, `point.x` and `point.y` semantically range from `0 ~ 8`
- Each grid cell is `1.0` wide and tall
- Mapping into the grid can then follow cluster-specific rules

This design supports:

- MPE
- Mapping a continuous touch area into an 8x8 grid
- Non-integer touch positions

## Multi-Touch Rules
If multi-touch is supported later, the rules are:

- A `TouchArea` cluster can reserve a fixed number of touch slots
- For example, `inputCount = 10` means the cluster supports up to 10 concurrent touches
- `localIndex = 0..9` represents the 10 touch slots
- Each slot corresponds to its own `TouchPointInfo`

Important rule:

- Once a touch is assigned to a slot, that slot must remain stable until release
- Do not reorder active touches into slots every frame
- A new touch claims a free slot
- A slot can only be reused after the touch assigned to it is released

This guarantees:

- Stable `InputId`
- Stable gesture tracking
- No MPE mapping instability caused by slot reshuffling

## Current Implementation Scope
Current priority:

1. Finish the type layer in `OS/Framework/Input`
2. Build the `MatrixOS::Input` service layer
3. Add the device bridge
4. Migrate `UI` and `Applications` last

Explicitly not in scope right now:

- UI migration
- Application migration
- Full replacement of old `KeyEvent / KeyInfo / MatrixOS::KeyPad`

## TODO

### Phase 1: OS Core
- [x] Add `InputId`
- [x] Add `InputClass`
- [x] Add `InputCluster`
- [x] Add `InputEvent`
- [x] Add `InputSnapshot`
- [x] Define the initial `XXXInfo` types
- [x] Add compile-time `sizeof <= 16B` and trivially copyable constraints to all `XXXInfo`
- [x] Add the `MatrixOS::Input` namespace and public header entry
- [x] Add the input event queue and state cache
- [x] Implement `Get()` / `GetState()` / `GetClusters()` / `GetCluster()`
- [x] Implement `GetPrimaryGridCluster()` / `GetPrimaryGridSize()`
- [x] Implement `GetInputsAt()` / `GetInputAt()` / `TryGetPoint()`

### Phase 2: Device Bridge
- [x] Build the new `InputEvent` reporting path between device drivers and `OS`
- [x] Register cluster descriptions for existing devices
- [x] Implement cluster coordinate mapping for existing devices
- [x] Move input coordinate rotation logic from `OS` into the device layer
- [x] Implement capabilities query APIs for existing devices

### Phase 3: Compatibility Layer
- [x] Keep the old `MatrixOS::KeyPad` API as a transition layer
- [x] Allow old `KeyEvent / KeyInfo` to be converted from the new input service
- [x] Keep existing logic such as `FUNCTION_KEY` working during migration

### Phase 4: UI Migration
- [x] Move `UI` from `KeyEvent` to `InputEvent`
- [x] Move `UIComponent` from `KeyInfo*` to the new input model
- [x] Keep current grid UI behavior unchanged

### Phase 5: Application Migration
- [x] Migrate `KeyPad` dependencies under `Applications/`
- [x] Migrate direct uses of `Device::xSize / ySize`
- [x] Migrate direct uses of `Device::KeyPad::*`

### Phase 6: Cleanup
- [ ] Remove the old public `MatrixOS::KeyPad` input model
- [ ] Remove direct use of old `KeyEvent / KeyInfo` in applications
- [ ] Remove the compatibility adapter layer
