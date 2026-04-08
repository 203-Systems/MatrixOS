# 输入系统修订 1

## 目的
这份文档记录对当前 `InputSystem` 方案的第一轮修正。

本轮修正重点只有两个：

- 旋转责任边界
- cluster 坐标映射责任边界

结论很明确：

- `Rotation` 事务不应该继续由 `OS` 主导
- `memberId <-> XY` 的换算不应该由 `OS/Input` 用通用公式硬编码

## 当前问题

当前分支实现虽然已经把 `rotation` 字段和 cluster metadata 放进了 `InputCluster`，但核心路径仍然是：

- `MatrixOS::SYS::Rotate()` 改全局 rotation
- `OS/Input` 收到通知
- `OS/Input` 自己做 `Point` 的旋转
- `OS/Input` 自己用 `memberId -> x/y -> Point`
- `OS/Input` 自己用 `Point -> memberId`

这有几个问题：

1. 责任边界错误
   - 旋转本质上是设备布局问题，不是通用 OS 行为

2. `memberId` 被 OS 假定成规则布局
   - 现在默认是 `Grid2D => y * width + x`
   - `Linear1D => x` 或 `y`
   - 这只适合最简单的规则矩阵

3. 不适合未来输入类型
   - 不规则物理布局
   - 有空洞的矩阵
   - TouchArea
   - Multi-touch slot
   - MPE
   - 任何未来不是规则行优先布局的 cluster

4. 容易和旧 `KeyPad` 路径双重旋转
   - 新 Input 路径旋一次
   - 旧 `KeyPad::XY2ID/ID2XY` 再旋一次

5. `FunctionKey` 不应该写死在 `InputId`
   - `InputId{0, 0}` 是当前某些设备的实现细节
   - 这不属于 framework 层的通用语义
   - `InputId::FunctionKey()` 和 `InputId::IsFunctionKey()` 这种硬编码会把设备拓扑重新写回 OS

6. `scanRateHz` 和 `pressureMax` 不应该进入通用 capabilities
   - 它们都不是上层真正需要依赖的通用输入能力
   - `scanRateHz` 是设备内部实现细节，通常由设备层自己的 task / scan loop 决定
   - `pressureMax` 也是设备内部量程细节，不应要求应用层理解和依赖
   - 这类信息如果以后确实需要，应放到 device-specific diagnostics / debug API，而不是通用 input capability

7. `RegisterInputClusters()` 这类主动注册模型不够合适
   - 它和当前仓库里 `LEDPartition` 的设备层数据提供方式不一致
   - 容易引入初始化顺序、重复注册、漏清理、旋转后重新注册等额外复杂度
   - cluster 数据更适合像 `LED::partitions` 一样由设备层直接持有，OS 只读取

## 修正后的核心原则

### 1. 删除 `MatrixOS::SYS::Rotate()`
`Rotation` 不再作为 OS 公共事务存在。

应删除：

```cpp
MatrixOS::SYS::Rotate(Direction rotation, bool absolute = false);
```

改为设备层入口：

```cpp
namespace Device
{
void Rotate(Direction rotation, bool absolute = false);
}
```

如果系统层需要触发旋转，也应直接调用 `Device::Rotate(...)`。

### 1.1 `FunctionKey` 定义下放到设备层
`FunctionKey` 不是 `InputId` 类型本身的一部分，而是设备层定义的特殊输入。

因此：

- 不应在 `InputId` 里继续保留 `FunctionKey()` 这类硬编码 helper
- 不应在 `InputId` 里继续保留 `IsFunctionKey()` 这类硬编码判断

建议改为设备层提供：

```cpp
namespace Device
{
InputId GetFunctionKeyId();
bool IsFunctionKey(InputId id);
}
```

如果需要给上层一个稳定入口，也可以由 `MatrixOS::Input` 做一层转发：

```cpp
namespace MatrixOS::Input
{
InputId GetFunctionKeyId();
bool IsFunctionKey(InputId id);
}
```

但真正的定义来源必须是设备层。

### 1.2 收紧通用 capabilities
通用 input capability 应只保留“上层行为差异真正需要知道的能力”。

因此：

- `scanRateHz` 不应保留在 `KeypadCapabilities`
- `pressureMax` 不应保留在 `KeypadCapabilities`

推荐保留的方向是：

```cpp
struct KeypadCapabilities {
  bool hasPressure;
  bool hasAftertouch;
  bool hasVelocity;
  bool hasPosition;
};
```

如果未来确实需要观测 scan rate、量程、滤波参数等信息，应通过设备诊断接口提供，而不是进入通用输入抽象。

### 1.3 `InputId` 命名修订
`localIndex` 这个名字容易误导人为：

- 顺序数组下标
- OS 可推导索引
- 默认可以用 `y * width + x` 这类规则公式换算

同时 `localId` 在很多字体里又容易和大写 `I` 混淆，不够清晰。

因此本轮修订建议统一改成：

```cpp
struct InputId {
  uint8_t clusterId;
  uint16_t memberId;
};
```

语义：

- `clusterId` 表示属于哪个 cluster
- `memberId` 表示该 cluster 内由设备定义的成员标识

`memberId` 应视为 opaque id，而不是默认的数组 index。

### 1.4 InputCluster 数据归设备层持有
`InputCluster` 的组织方式不应继续走 `RegisterInputClusters()` 这种主动注册模式。

更适合当前仓库风格的方式是：

- 设备层直接持有 cluster 数据
- 设备层直接持有 cluster mapping handler
- OS 只读取和调度

这和当前 `LEDPartition` 的组织方式更一致。

推荐方向：

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

关键点：

- cluster 数据来源于设备层
- 坐标映射来源于设备层
- OS 不负责注册，不负责推导，不负责解释布局

### 1.5 cluster id 不做 OS 统一硬编码
不建议由 OS 规定所有设备都必须使用同一组 cluster id。

更合理的方式是：

- 每个 device / family 自己定义 cluster id 常量
- OS 只把 `clusterId` 当 opaque id
- 只有确实需要跨设备统一访问的特殊输入，才通过设备层 API 暴露

例如：

- `FunctionKey`
- 主 grid cluster

都应通过设备层查询接口获取，而不是由 OS 假设：

- `MainGrid = 1`
- `FunctionKey = {0, 0}`

## 2. 旋转事务完全归设备层负责
设备层负责完整的旋转事务，不只是更新一个方向值。

设备层应统一负责：

- 更新当前设备 rotation 状态
- 重新注册 input clusters
- 重建 cluster 坐标映射
- 清理输入事件队列
- 清理输入状态缓存
- 清理旧 keypad 队列或兼容缓存
- 旋转或清空 LED buffer
- 处理 touch / MPE / scan 相关缓存
- 处理任何设备特有的附带状态

这里的关键点不是“设备层自己去实现所有底层功能”，而是：

- **由设备层负责 orchestration**
- OS 可以提供基础 helper
- 但不要由 OS 发起和主导 rotation 事务

## 3. `memberId` 是设备定义的，不是 OS 定义的
`InputId.memberId` 的含义必须保持为：

- 设备定义的 cluster 内输入标识

而不是：

- OS 默认推导出来的规则矩阵索引

也就是说：

- `memberId -> Point`
- `Point -> memberId`

都必须是 **device-assigned handler**

不能再由 `OS/Input` 写成通用公式：

```cpp
localX = memberId % width;
localY = memberId / width;
```

除非某个设备自己决定这样做。

## 4. OS/Input 不再负责坐标旋转和坐标换算
`OS/Input` 的职责应收缩为：

- 输入事件队列
- 输入状态缓存
- cluster 注册表
- 对设备层 mapping handler 的统一调度

`OS/Input` 不应该再做：

- 根据 rotation 自己旋转 `Point`
- 根据 `memberId` 自己推 `x/y`
- 根据 `Point` 自己算 `memberId`

## 修正后的建议模型

### InputCluster 只保留描述数据
`InputCluster` 保留这些元数据即可：

- `clusterId`
- `name`
- `inputClass`
- `shape`
- `rootPoint`
- `dimension`
- `inputCount`

它不应该再承载“OS 自己就能完成映射”的隐含语义。

### 坐标映射改为 handler 模型
推荐让设备层为每个 cluster 提供映射 handler。

例如：

```cpp
struct InputClusterOps {
  bool (*TryGetPoint)(uint16_t memberId, Point* point);
  bool (*TryGetMemberId)(Point point, uint16_t* memberId);
};
```

或者用等价的设备层 API：

```cpp
bool Device::Input::TryGetPoint(uint8_t clusterId, uint16_t memberId, Point* point);
bool Device::Input::TryGetMemberId(uint8_t clusterId, Point point, uint16_t* memberId);
```

重点不是具体形式，而是责任归属：

- **映射由设备层决定**
- **OS 只负责调用**

## 修正后的 API 语义

### `MatrixOS::Input::TryGetPoint`
改为：

- 查 cluster
- 调设备层 handler
- 不做通用公式换算

### `MatrixOS::Input::GetInputAt`
改为：

- 查 cluster
- 调设备层 `TryGetMemberId`
- 不做通用公式换算

### `MatrixOS::Input::GetInputsAt`
改为：

- 遍历所有支持坐标的 cluster
- 逐个调用该 cluster 的 device handler
- 收集命中的 `InputId`

### `MatrixOS::KeyPad::XY2ID / ID2XY`
兼容层也不应再自行旋转。

应改为：

- 委托给新的 `MatrixOS::Input` / device mapping
- 或直接走 device handler
- 不能再叠加一层独立的 OS rotation

## Buffer 处理约定

### 输入相关 buffer
旋转发生时，设备层应优先清理：

- Input event queue
- Input state cache
- 旧 KeyPad queue
- 旧 KeyInfo 状态
- TouchArea / MPE / multi-touch slot 状态

原因：

- 旋转后坐标语义已经变了
- 旧状态继续沿用通常是错的

### LED buffer
LED buffer 的处理策略由设备层决定：

- rotate existing buffer
- clear buffer
- 或按设备能力选择混合策略

但决策和触发点必须在设备层，不在 OS。

## 对当前 InputSystem 的直接修订

这轮修订后，原方案中以下表述需要视为过时：

1. `MatrixOS::SYS::Rotate()` 作为长期公共 API
2. `OS/Input` 自己处理 cluster rotation
3. `OS/Input` 自己按 `Grid2D/Linear1D` 推导 `memberId`
4. `InputId` 自己硬编码 `FunctionKey`
5. 通用 `KeypadCapabilities` 暴露设备实现细节如 `scanRateHz`、`pressureMax`
6. 使用 `RegisterInputClusters()` 作为长期 cluster 生命周期模型

新的硬规则是：

- 旋转入口在设备层
- cluster mapping 在设备层
- `memberId` 是设备定义的 opaque identity
- `FunctionKey` 是设备定义的特殊输入，不属于 framework 的固定常量
- 通用 capabilities 只暴露上层真正需要的行为能力，不暴露设备实现参数
- `InputCluster` 数据和 mapping handler 都归设备层持有
- OS 只调度，不解释布局

## 下一轮实现要求

### 第一优先级
- 删除 `MatrixOS::SYS::Rotate()`
- 新增 `Device::Rotate(...)`
- 把 rotation 事务完整收口到设备层

### 第二优先级
- 从 `OS/Input` 删除通用 `memberId <-> Point` 推导逻辑
- 改成设备层 handler 模型

### 第三优先级
- 将 `InputId.localIndex` 全部改名为 `memberId`
- 明确 `memberId` 是 device-defined opaque id

### 第四优先级
- 从 `InputId` 移除硬编码的 `FunctionKey` helper
- 改为设备层提供 `GetFunctionKeyId / IsFunctionKey`

### 第五优先级
- 收紧 `KeypadCapabilities`，移除 `scanRateHz` 和 `pressureMax`

### 第六优先级
- 去掉 `RegisterInputClusters()` 模型
- 改为 device-owned cluster table + device-owned mapping ops

### 第七优先级
- 修复旧 `KeyPad` 兼容层的双重旋转问题
- 让旧路径也统一走新 mapping

## 一句话总结
这轮修订的核心不是“再加一套 rotation callback”，而是：

**把 rotation 和 mapping 真正从 OS 收走，交还给设备层。**
