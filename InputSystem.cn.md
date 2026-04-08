# 通用输入系统设计

## 摘要
将现有 `KeyEvent / KeyInfo / MatrixOS::KeyPad` 输入体系，逐步重构为以 `InputId / InputEvent / InputSnapshot / InputCluster / MatrixOS::Input` 为核心的通用输入体系。

当前阶段只先落 `OS/Framework/Input` 的类型层与设计约定，`UI` 和 `Applications` 暂不迁移。

## 当前已锁定的设计

### 输入身份
输入身份统一为 `InputId`：

```cpp
struct InputId {
  uint8_t clusterId;
  uint16_t localIndex;
};
```

约定：

- 不再保留独立的全局扁平 `id`
- `clusterId + localIndex` 就是输入的唯一身份
- `InputEvent` 和 `InputSnapshot` 中字段名统一为 `id`

### 事件与快照
不保留独立 `InputInfo` 包装层。事件和快照直接携带：

- `id`
- `inputClass`
- `union(XXXInfo)`

当前结构：

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

约定：

- `XXXInfo` 使用 plain struct
- 不使用继承
- 不使用虚函数
- 不使用 `std::variant`
- 每个 `XXXInfo` 编译期强制 `<= 16B`
- 每个 `XXXInfo` 必须 trivially copyable
- `InputEvent` 与 `InputSnapshot` 也必须 trivially copyable

## InputClass
当前已定义的输入类别：

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

说明：

- `TouchScreen` 已统一改名为 `TouchArea`
- `TouchStrip` 当前已移除
- `Generic` 暂时保持最简单设计，只提供一个 `uint64_t value`

## InputCluster
`InputCluster` 只描述结构与定位，不内嵌 capabilities。

当前字段：

- `clusterId`
- `name`
- `inputClass`
- `shape`
- `rootPoint`
- `dimension`
- `inputCount`

约定：

- 没有物理坐标时，`rootPoint = Point::Invalid()`
- 是否支持坐标映射，主要由 `shape`、`rootPoint` 和 cluster 自己的映射逻辑决定
- 不假设所有输入都具备物理坐标

### 旋转责任归属
后续设备旋转事务从 `OS` 下放到设备层实现。

约定：

- `OS` 不再负责做统一的全局输入旋转
- 设备层负责根据当前设备方向，产出最终有效的 cluster 坐标映射
- `InputCluster.rootPoint`
- `InputCluster.dimension`
- `TryGetPoint(localIndex, Point*)`
- `TryGetLocalIndex(Point, uint16_t*)`
  这些都应当体现设备当前方向后的结果
- `MatrixOS::Input` 与上层 `UI / Applications` 看到的，始终是设备层已经处理完成后的最终坐标

这样做的目的：

- 避免 `OS` 里维护一套全局旋转状态
- 避免不同设备 family 在 `OS` 层做额外特判
- 让输入映射和设备物理布局保持同一责任边界
- 后续若 LED、输入、触摸区域都需要跟随方向变化，设备层更容易统一实现

## Capabilities
capabilities 不放在 `InputCluster` 结构体里，而是通过 API 获取。

原则：

- 每个 `InputClass` 走自己独立的强类型 capabilities 结构
- 不做通用 bitmask
- 不做通用键值表
- 不是所有类型都必须有 capabilities 结构

查询方式示意：

```cpp
bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps);
bool GetFaderCapabilities(uint8_t clusterId, FaderCapabilities* caps);
bool GetEncoderCapabilities(uint8_t clusterId, EncoderCapabilities* caps);
```

调用方式：

1. 先通过 `GetCluster(clusterId)` 看 `inputClass`
2. 再调用对应 class 的 capabilities API

静态能力与可调配置继续分开：

- 静态能力走 `GetXxxCapabilities`
- 可调参数走 `GetXxxConfig / SetXxxConfig`

## MatrixOS::Input 公共 API
目标公开接口统一迁移到 `MatrixOS::Input`：

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

语义约定：

- `GetInputsAt(Point)` 使用全局坐标，返回所有命中的输入
- `GetInputAt(clusterId, Point)` 也使用全局坐标，但只解析指定 cluster
- `TryGetPoint(InputId, Point*)` 用于单个输入反查全局坐标
- 第一阶段仍保留 `GetPrimaryGridSize()` 这类 grid helper，方便旧系统迁移

## 当前各类 Info 约定

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

说明：

- Fader 不保留 `Touched`
- Fader 不保留 `Released`

### Encoder
- `EncoderState`
  - `Idle`
  - `Decrement`
  - `Increment`
- `EncoderInfo`
  - `lastEventTime`
  - `state`

说明：

- 不再使用 `Left / Right`
- 不保留 `Pressed`

### TouchArea / TouchPoint
`TouchArea` 是输入类别名，具体 payload 类型名使用 `TouchPointInfo`。

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

说明：

- 不使用离散 `Point`
- `point` 使用 `PointFloat`
- 不保留 `primaryTouch`

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

说明：

- 当前不保留 `TemperatureCapabilities`

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

说明：

- `value` 直接使用 `uint64_t`
- 目前不做动态 layout

## TouchArea 坐标与 MPE 约定
`TouchPointInfo.point` 的语义定义为：

- cluster 内局部连续坐标
- 范围不是固定 `0 ~ 1`
- 范围由该 cluster 的 `dimension` 决定

例如：

- 对 8x8 的 `TouchArea`，`point.x` 和 `point.y` 的语义就是 `0 ~ 8`
- 每个格子的宽高都是 `1.0`
- 映射到 grid 时可按 cluster 规则转换

这套设计是为了后续支持：

- MPE
- 连续触摸区域映射到 8x8 grid
- 非整数触点位置

## Multi-Touch 约定
后续若支持 multi-touch，约定如下：

- 一个 `TouchArea` cluster 可以预留固定数量的 touch slot
- 例如 `inputCount = 10`，表示该 cluster 最多支持 10 个并发触点
- `localIndex = 0..9` 就是 10 个触点槽位
- 每个槽位各自对应一个 `TouchPointInfo`

重要规则：

- 触点一旦分配到某个 slot，在释放前必须保持稳定
- 不允许每一帧把当前触点重新按顺序重排到 slot 中
- 新触点出现时分配空闲 slot
- 触点释放后，对应 slot 才能被复用

这样可以保证：

- `InputId` 稳定
- gesture 识别不乱
- 后续 MPE 映射不会因为 slot 抖动而失效

## 当前实施范围
当前优先级：

1. 先完成 `OS/Framework/Input` 的类型层
2. 再做 `MatrixOS::Input` 的服务层
3. 再接设备桥接
4. 最后再迁移 `UI` 和 `Applications`

当前明确不做：

- UI 迁移
- Application 迁移
- 旧 `KeyEvent / KeyInfo / MatrixOS::KeyPad` 的全面替换

## TODO

### Phase 1: OS Core
- [x] 新建 `InputId`
- [x] 新建 `InputClass`
- [x] 新建 `InputCluster`
- [x] 新建 `InputEvent`
- [x] 新建 `InputSnapshot`
- [x] 建立各类 `XXXInfo` 基础定义
- [x] 为所有 `XXXInfo` 添加 `sizeof <= 16B` 与 trivially copyable 编译期约束
- [ ] 新建 `MatrixOS::Input` 命名空间与头文件入口
- [ ] 新建输入事件队列与状态缓存
- [ ] 实现 `Get()` / `GetState()` / `GetClusters()` / `GetCluster()`
- [ ] 实现 `GetPrimaryGridCluster()` / `GetPrimaryGridSize()`
- [ ] 实现 `GetInputsAt()` / `GetInputAt()` / `TryGetPoint()`

### Phase 2: Device Bridge
- [ ] 在设备驱动与 OS 之间建立新的 `InputEvent` 上报路径
- [ ] 为现有设备注册 cluster 描述
- [ ] 为现有设备实现 cluster 坐标映射接口
- [ ] 将输入坐标旋转逻辑从 OS 下放到设备层
- [ ] 为现有设备实现各类 capabilities 查询接口

### Phase 3: Compatibility Layer
- [ ] 保留旧 `MatrixOS::KeyPad` API 作为过渡兼容层
- [ ] 让旧 `KeyEvent / KeyInfo` 可以从新输入服务转换
- [ ] 保持 `FUNCTION_KEY` 等现有逻辑在迁移期继续可用

### Phase 4: UI Migration
- [ ] 将 `UI` 从 `KeyEvent` 切换到 `InputEvent`
- [ ] 将 `UIComponent` 从 `KeyInfo*` 切换到新的输入模型
- [ ] 保持当前 grid UI 行为不回退

### Phase 5: Application Migration
- [ ] 批量迁移 `Applications/` 下的 `KeyPad` 依赖
- [ ] 批量迁移 `Device::xSize / ySize` 的直接使用
- [ ] 批量迁移 `Device::KeyPad::*` 的直接使用

### Phase 6: Cleanup
- [ ] 删除旧公开 `MatrixOS::KeyPad` 输入模型
- [ ] 删除旧 `KeyEvent / KeyInfo` 在应用层的直接使用
- [ ] 清理兼容适配层
