# 输入系统修订 2

## 目的
这份文档记录当前输入系统方案的第二轮修订。

本轮修订聚焦 4 个问题：

- `memberId` 到底是稠密的还是完全 opaque 的
- 现有基于 `Point` 的 API 到底适用于哪些 cluster
- `TouchArea` 是否适合当前的 `InputClusterShape`
- cluster mapping handler 应该放在哪里

## 结论

- `memberId` 应该是 **稳定且在 cluster 内稠密的**
- `GetInputAt / GetInputsAt / TryGetPoint` 只应该适用于 **离散可坐标寻址的 cluster**
- `TouchArea` 不应该被强行塞进 `Grid2D` 的语义里
- mapping handler 应该 **直接并入 `InputCluster`**

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

## 对修订 1 的直接补充

在修订 1 的基础上，再增加以下硬规则：

1. `memberId` 必须落在 `[0, inputCount)` 内
2. `memberId` 仍然由设备定义且保持稳定，但 OS 不能从它推导布局
3. `GetInputAt / GetInputsAt / TryGetPoint` 仅服务离散可坐标寻址 cluster
4. `TouchArea` 应使用连续二维 shape，例如 `Area2D`，而不是 `Grid2D`
5. mapping handler 应直接并入 `InputCluster`

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
- 把修订 1 和修订 2 的结论折回主文档 `InputSystem.md`

## 一句话总结

本轮修订的核心是：

**让 `memberId` 既稠密又 opaque，把整数 `Point` API 限定在离散 cluster 上，并把 mapping handler 直接绑定到每个 `InputCluster`。**
