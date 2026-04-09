# 输入系统修订 2

## 目的
这份文档记录当前输入系统方案的第二轮修订。

本轮修订聚焦 4 个问题：

- `memberId` 到底是稠密的还是完全 opaque 的
- 现有基于 `Point` 的 API 到底适用于哪些 cluster
- `TouchArea` 是否适合当前的 `InputClusterShape`
- cluster mapping handler 应该放在哪里
- 旧 `KeyPad` 兼容层到底应该保留到什么程度
- `MatrixOS::SYS::Rotate()` 是否还能继续作为公开 wrapper
- `MatrixOS::Input` 是否仍然过于 keypad-centric
- `InputCluster` 里是否还应该保留 rotation 实现细节

## 结论

- `memberId` 应该是 **稳定且在 cluster 内稠密的**
- `GetInputAt / GetInputsAt / TryGetPoint` 只应该适用于 **离散可坐标寻址的 cluster**
- `TouchArea` 不应该被强行塞进 `Grid2D` 的语义里
- mapping handler 应该 **直接并入 `InputCluster`**
- 旧 `KeyPad` 层只能作为 **短生命周期的迁移 shim**
- `MatrixOS::SYS::Rotate()` 即使暂时保留，也只能是转发到 `Device::Rotate()` 的过渡入口
- `MatrixOS::Input` 的核心层不应该长期围绕 keypad/grid helper 展开
- 一旦 mapping 完全归设备层，`InputCluster` 就不应该继续保存 rotation 实现字段

## 1. `memberId` 必须稳定且稠密

修订 1 已经把 `memberId` 的语义改对了：它是 device-defined identity，不是 OS 推出来的布局索引。

但这里还缺一个关键约束：

- `memberId` 是不是可以完全 opaque 且稀疏
- 还是应该虽然由设备定义，但依然落在 `[0, inputCount)` 内

本轮修订把这一点定死：

- `memberId` 仍然由设备层定义
- `memberId` 仍然不能被 OS 用公式解释布局
- 但是 `memberId` **必须**落在 `[0, inputCount)` 内
- 并且同一个 member 生命周期内 `memberId` **必须稳定**

这样可以同时满足两边需求：

- 设备层仍然拥有语义解释权
- OS 仍然可以安全地分配按 cluster 的状态数组
- touch slot 可以有稳定 id
- event queue 和 state cache 依然能保持简单

### 这意味着什么

例如：

- 8x8 主矩阵可以用 `memberId = 0..63`
- 10 指触摸区可以用 `memberId = 0..9`
- 4 通道推子可以用 `memberId = 0..3`

但 OS **不能**再默认：

```cpp
x = memberId % width;
y = memberId / width;
```

OS 唯一允许有的通用假设是：

- `0 <= memberId < inputCount`
- 同一个物理/逻辑 member 的 `memberId` 保持稳定

## 2. 基于 `Point` 的 API 只适用于离散可坐标寻址 cluster

当前这组 API：

```cpp
void GetInputsAt(Point xy, vector<InputId>* ids);
bool GetInputAt(uint8_t clusterId, Point xy, InputId* id);
bool TryGetPoint(InputId id, Point* xy);
```

是有用的，但它们只适用于一种 cluster：

- 离散的
- 能用整数坐标寻址的

本轮修订明确把这点写死。

这组 API 适合：

- `Keypad`
- 离散 `Linear1D`
- 以后任何离散 grid / 离散条带输入

这组 API **不适合**：

- `TouchArea`
- 连续表面输入
- multi-touch 空间数据
- 任何天然坐标不是整数 `Point` 的 cluster

### 规则

`GetInputAt / GetInputsAt / TryGetPoint` 应该被明确文档化和实现为：**仅适用于离散可坐标寻址 cluster 的 API**。

不要把它们伪装成适用于所有 `InputClass` 的通用空间查询接口。

## 3. `TouchArea` 需要不同的 shape 语义

当前的 shape enum 是：

```cpp
enum class InputClusterShape : uint8_t {
  Scalar = 0,
  Linear1D,
  Grid2D,
};
```

这对离散 cluster 是够的，但 `TouchArea` 不一样：

- 它是连续的
- 它可能支持 multi-touch
- 它可能上报 `PointFloat`
- 它不应该暗示存在整数 `Point <-> memberId` 映射

因此，`TouchArea` 不应该被当成普通 `Grid2D`。

### 推荐方向

增加一个连续二维 shape，例如：

```cpp
enum class InputClusterShape : uint8_t {
  Scalar = 0,
  Linear1D,
  Grid2D,
  Area2D,
};
```

含义：

- `Grid2D` 表示离散二维可寻址 member
- `Area2D` 表示连续二维空间输入

这样语义会更干净：

- `Keypad` 属于 `Grid2D`
- `TouchArea` 属于 `Area2D`

`TouchArea` 仍然可以为了 touch slot 使用稠密 `memberId`，但这 **不代表**它应该参与和 `Keypad` 一样的整数坐标 API。

## 4. Mapping Handler 应该直接放进 `InputCluster`

修订 1 已经否定了 `RegisterInputClusters()` 这种主动注册模型。

剩下还没定死的一点是：

- mapping handler 应该放在单独的 `clusterOps` 表里
- 还是应该直接并进 `InputCluster`

本轮修订选第二种。

### 为什么

像这种并行表：

```cpp
extern vector<InputCluster> clusters;
extern vector<ClusterMappingOps> clusterOps;
```

会带来没必要的同步问题：

- 两张表必须永远严格对齐
- 旋转重建时可能一边改顺序另一边没改
- 缺失 ops 时语义不清楚
- 运行时一旦错位，就会变成 cluster A 用 cluster B 的 mapping

更简单也更安全的方式是：

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

### 规则

- cluster 描述数据和离散 mapping 入口放在一起
- 设备层拥有并填充完整 `InputCluster`
- OS 只读取和调度
- 不再保留平行的 `clusterOps` 表

### 一个重要限制

这两个 handler 仍然只适用于离散可坐标寻址 cluster。

对于 `TouchArea / Area2D`：

- 这两个指针可以是 `nullptr`
- 以后如果需要连续空间 API，再单独加，不要现在强行复用

## 5. 旧 `KeyPad` 层不能长期作为第二套真实输入系统存在

当前分支里，设备侧真实路径仍然是：

- device driver 先产出 `KeyEvent / KeyInfo`
- `MatrixOS::KeyPad` 接收
- `MatrixOS::KeyPad` 再桥接成 `InputEvent`

这在迁移期可以接受，但不能成为最终形态。

### 规则

兼容层可以暂时存在，但必须被压缩成一个极薄 shim：

- 不能再有自己独立的 mapping 规则
- 不能再有自己独立的 rotation 逻辑
- 不能再有第二份状态缓存
- 不能再有第二个 source of truth

长期目标应当是 device driver 直接产出 `InputEvent`。

如果旧 `KeyPad` 层还存在，它只能向新系统转发，不能再自己解释设备拓扑。

## 6. `MatrixOS::SYS::Rotate()` 只能视为过渡，不是最终架构

修订 1 已经明确 rotation ownership 归设备层。

这意味着下面这种关系最多只应该是迁移便利：

```cpp
MatrixOS::SYS::Rotate(...)
  -> Device::Rotate(...)
```

它本身并不等于“rotation 已经真正回到设备层”。

### 规则

- 权威 rotation 入口是 `Device::Rotate(...)`
- `MatrixOS::SYS::Rotate()` 可以暂时做转发
- 但它不应该继续作为长期 public API 存在

否则后续新代码还是会继续把 rotation 当成 OS 事务来写。

## 7. `MatrixOS::Input` 核心层不能长期 keypad-centric

当前 `MatrixOS::Input` 里还保留了这些 helper：

- `GetPrimaryGridCluster()`
- `GetPrimaryGridSize()`
- `GetKeypadState(Point)`
- `HasVelocitySensitivity()`

这些在迁移期很有用，但它们不属于“通用输入系统”的核心语义。

### 规则

- 这些 API 应被视为迁移 helper
- 不应定义 `MatrixOS::Input` 的长期身份
- 通用核心应当保持在：
  - event queue
  - state cache
  - cluster enumeration
  - device-owned mapping dispatch

新的输入系统不能慢慢退化成“换了个名字的 KeyPad”。

## 8. `InputCluster` 不应永久保存 rotation 实现字段

如果最终架构是：

- rotation 归设备层
- mapping 归设备层
- 设备层直接产出最终旋转后的坐标

那么下面这些字段：

- `rotation`
- `rotationDimension`

更像是当前实现阶段的临时细节，而不是长期的 cluster metadata。

### 规则

- `InputCluster` 最终应只保留描述信息和 mapping 入口
- 仅用于当前实现的 rotation bookkeeping 应该收进设备层 mapping handler 内部

如果长期保留这些字段，就等于把某一套具体 mapping 实现策略重新泄漏回 framework 类型。

## 9. Mystrix1 的 bring-up 说明“只有事件上报”还不够

当前 Mystrix1 的实际表现已经验证了一个重要规则：

- 旧 keypad driver 仍然能产出事件
- 这些事件也确实被桥接成了 `InputEvent`
- 但如果设备侧没有 cluster mapping，普通 pad 事件仍然到不了 UI component

原因是：

- function key 可以绕过坐标查找，直接作为特殊键处理
- 普通 pad 事件要进入 UI，仍然必须先做 `InputId -> Point` 映射
- 如果 `Device::Input::TryGetPoint(...)` 没实现，grid 事件在新 UI 路径里就是“看不见的”

### 规则

对于迁移期仍然使用旧 keypad driver 的设备：

- 仅有 event bridge 是不够的
- 设备层还必须至少提供一个最小可用的 discrete cluster model 和 coordinate mapping

这意味着像 Mystrix1 这样的 bring-up，最低要求也必须有：

- function key cluster
- main grid cluster
- 必要的 touchbar cluster
- 可工作的 `TryGetPoint`
- 可工作的 `TryGetMemberId`

否则系统就会呈现“半活”状态：

- FN 能用
- 普通 pad 不能用

这不只是一个实现 bug，而是一个结构性提醒：

新的输入系统依赖两件事同时成立：

- event production
- coordinate resolution

## 对修订 1 的直接补充

在修订 1 的基础上，再增加以下硬规则：

1. `memberId` 必须落在 `[0, inputCount)` 内
2. `memberId` 仍然由设备定义且保持稳定，但 OS 不能从它推导布局
3. `GetInputAt / GetInputsAt / TryGetPoint` 仅服务离散可坐标寻址 cluster
4. `TouchArea` 应使用连续二维 shape，例如 `Area2D`，而不是 `Grid2D`
5. mapping handler 应直接并入 `InputCluster`
6. 旧 `KeyPad` 层只是临时迁移 shim，不再是长期输入 owner
7. `MatrixOS::SYS::Rotate()` 如果暂时保留，也只是过渡包装；真正权威入口是 `Device::Rotate()`
8. `MatrixOS::Input` 中 keypad/grid helper 只是迁移工具，不是核心语义
9. `InputCluster` 最终应移除 rotation 实现字段
10. 迁移期设备除了 event bridge 之外，还必须补齐最小 cluster/mapping，特殊键之外的普通 pad 才会工作

## 下一步实现优先级

### Priority 1
- 把 `localIndex` 改名为 `memberId`
- 明确文档写清楚 `memberId` 落在 `[0, inputCount)` 内
- 保持 `memberId` 设备定义且稳定

### Priority 2
- 收紧 `GetInputAt / GetInputsAt / TryGetPoint` 的语义，只服务离散可坐标寻址 cluster
- 确保 `TouchArea` 不依赖这组 API

### Priority 3
- 增加连续二维 shape，例如 `Area2D`
- 把 `TouchArea` 归到这个 shape

### Priority 4
- 把离散 mapping handler 并进 `InputCluster`
- 去掉平行 `clusterOps` 表的设计

### Priority 5
- 把 `MatrixOS::KeyPad` 收缩成极薄的临时 shim
- 停止在里面继续新增逻辑

### Priority 6
- 保持 `Device::Rotate(...)` 作为唯一权威 rotation 入口
- 规划移除 `MatrixOS::SYS::Rotate()` 的长期 public API 地位

### Priority 7
- 把 keypad/grid convenience API 标记为迁移 helper
- 不让它们进入长期核心抽象

### Priority 8
- 把 rotation bookkeeping 收进设备层 mapping handler
- 逐步移除 `InputCluster` 里的 `rotation` / `rotationDimension`

### Priority 9
- 对 Mystrix1 这类迁移期设备，先补齐最小 cluster + mapping，再期待 UI 层对齐

### Priority 10
- 把修订 1 和修订 2 的结论折回主文档 `InputSystem.md`

## 一句话总结

本轮修订的核心是：

**让 `memberId` 既稠密又 opaque，把整数 `Point` API 限定在离散 cluster 上，把 mapping handler 直接绑定到每个 `InputCluster`，并避免让 keypad 时代的过渡 API 定义长期架构。**
