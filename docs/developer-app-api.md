# DeveloperApp API 草案

## 目标

DeveloperApp 是一个给调试工具用的 bridge app。它只做三件事：

- 从 HID 或 MIDI 接收 LED write command。
- 把设备上的 key input report 回传给 host。
- 提供一个可靠的 clear screen command。

接口设计优先考虑简单直接。Host 不应该为了点亮一个 LED 就要理解 MatrixOS 的内部结构。

## Transport

### HID

HID API 常开。Host 可以随时发送 LED / key read / input report control command。

`PING` 只用于测试连通性和读取基本设备信息，不负责启用或关闭 API。

DeveloperApp 不提供本机按键退出方式。退出只能由外部 API 发送 `EXIT_APP`。

### MIDI

MIDI API 常开。普通 MIDI note / CC 可以直接控制 LED。

以下消息会打开 MIDI default key report：

- 收到 MIDI realtime `Start`
- 收到 MIDI realtime `Continue`

SysEx `SET_INPUT_REPORT, KEY_INFO, 1` 会打开 SysEx `INPUT_EVENT` report。SysEx `PING` 只用于测试连通性和读取基本设备信息。

## LED Address

LED 支持两种地址：

- `index`：LED 的全局编号，范围是 `0..LED_COUNT-1`。这个应该覆盖主网格、underglow 和其他 LED。
- `xy`：有符号坐标，适合主 8x8 或向外扩展的 10x10/underglow 显示。

HID 中 `x` / `y` 直接按 `int8_t` 解释。

SysEx 中 `x` / `y` 使用 signed 7-bit，也就是 `int7_t`：

- `0..63` 表示 `0..63`
- `64..127` 表示 `-64..-1`

例子：

- `0` 表示 `0`
- `1` 表示 `1`
- `64` 表示 `-64`
- `127` 表示 `-1`

XY LED command 不提供 row / column wildcard。需要一次写多个 LED 时，使用 `LED_WRITE_*_BULK` command。

## LED Command 分区

命令按范围分组：

- `0x00..0x0F`：session / ping / control。
- `0x10..0x2F`：LED write / clear / canvas update。
- `0x30..0x3F`：Key read。
- `0x80..0x8F`：Device to host reply。
- `0x90..0x9F`：Device to host async report。

## LED Write Flags

LED write command 使用 `flags` 控制写入行为和颜色格式。

| Bit | Name | Meaning |
| --- | --- | --- |
| `0` | `CANVAS` | 写入 canvas，但不 update |
| `1` | `RGBW` | payload 中每个 color 都包含 W |

规则：

- `CANVAS = 0`：写完后立即 update。Bulk write 会在所有 entry 写完后 update 一次。
- `CANVAS = 1`：只写 canvas，不 update。Host 需要之后发送 `LED_CANVAS_UPDATE`。
- `RGBW = 0`：颜色格式是 RGB，`w = 0`。
- `RGBW = 1`：颜色格式是 RGBW。
- 普通 LED write 只包含一个 entry。Bulk LED write 使用 `[flags, count, entries...]`。
- Bulk `count` 是 entry 数量，必须至少为 `1`。
- 未定义的 flag bit 必须为 `0`，否则回复 `BAD_PAYLOAD`。

## LED Clear Flags

`LED_CLEAR_SCREEN` 使用单独的 `flags`。

| Bit | Name | Meaning |
| --- | --- | --- |
| `0` | `REFRESH` | 清空后立刻 update |

规则：

- `REFRESH = 0`：只清空 canvas，不 update。
- `REFRESH = 1`：清空 canvas 后立刻 update。
- 未定义的 flag bit 必须为 `0`，否则回复 `BAD_PAYLOAD`。

## Reply Model

Device to host 只保留两种同步回复：

- `ACK`：命令成功，或命令已被接受。
- `ERROR`：命令失败。

异步 input report 单独走 `INPUT_EVENT`。

`ACK` 的 payload 格式：

```text
[ack_command, status, data...]
```

- `ack_command`：被回复的 host command。
- `status`：状态码，成功时为 `OK`。
- `data`：可选数据，由具体命令决定。

`PING` 回复：

```text
ACK, PING, OK, version, width, height, led_count
```

`KEY_READ_INDEX` / `KEY_READ_XY` 回复：

```text
ACK, KEY_READ_*, OK, input_event_type, input_data...
```

如果 key 不存在或坐标无效，则回复：

```text
ERROR, KEY_READ_*, BAD_PAYLOAD
```

## Input Report Control

Input report 需要单独开关，不和 LED API 绑在一起。

这个开关是 per transport 的：

- 通过 HID 发 `SET_INPUT_REPORT`，只影响 HID report。
- 通过 SysEx 发 `SET_INPUT_REPORT`，只影响 SysEx report。
- 普通 MIDI note / CC 没有 `SET_INPUT_REPORT` command；如果 host 只用普通 MIDI，不用 SysEx，就使用默认 MIDI 行为。

`0x01 SET_INPUT_REPORT` 用来打开或关闭某一种 input report：

```text
[type, enable]
```

- `type`：report 类型。
- `enable`：`0` 关闭，非 `0` 开启。

Report type:

| Type | Name | Meaning |
| --- | --- | --- |
| `0x00` | `ALL` | 所有已支持的 input report |
| `0x01` | `KEY_INFO` | 普通按键事件和按键状态 |
| `0x02` | `ANALOG_INFO` | 预留，之后给 analog / fader / encoder |
| `0x03` | `MPE_INFO` | 预留，之后给 MPE / touch position / per-key expression |

V1 只实现 `ALL` 和 `KEY_INFO`。`ALL` 在 V1 等同于 `KEY_INFO`，但之后新增 analog / MPE report 时，`ALL` 会一起控制所有类型。

HID 和 SysEx 的 `PING` 不影响 input report。Host 需要 input event 时，应该发送：

```text
SET_INPUT_REPORT, KEY_INFO, 1
```

MIDI realtime `Start` / `Continue` 是给 DAW 用的简易入口。为了易用，它会默认打开 `KEY_INFO` MIDI report。

## Input Event Payload

`0x90 INPUT_EVENT` 的第一个 payload byte 永远是 `event_type`。后面的 data 由 `event_type` 决定。

Event type:

| Type | Name | Data |
| --- | --- | --- |
| `0x01` | `KEY_INFO` | key data |
| `0x02` | `ANALOG_INFO` | 预留 |
| `0x03` | `MPE_INFO` | 预留 |

HID `KEY_INFO` data:

```text
[cluster, member_hi, member_lo, x_i8, y_i8, state, pressure_hi, pressure_lo, velocity_hi, velocity_lo]
```

SysEx `KEY_INFO` data:

```text
[cluster, member_lsb7, member_msb7, x_i7, y_i7, state, pressure7, velocity7]
```

`KEY_INFO` 的 key 身份由 `cluster + member` 决定，`x/y` 只是有坐标 input 的位置数据。Host 不应该只用 `x/y` 判断一个 key。

V1 key cluster:

| Cluster | Member | Meaning | XY |
| --- | --- | --- | --- |
| `0x00` | `0x0000` | Function Key | 忽略，当前填 `0, 0` |
| `0x01` | grid member id | Primary grid key | 有效 grid 坐标 |

其他 cluster 是保留或设备扩展。Host 如果只想 mirror 主网格，应只处理 `cluster = 0x01`。Host 如果想 mirror 实体设备输入到虚拟 Mystrix，应把 `cluster = 0x00, member = 0x0000` 映射到虚拟 Function Key。

`velocity` 的含义跟 `state` 有关：按下时是 strike velocity，释放时是 release velocity。

## HID API

RawHID report 固定 32 bytes。

Host 发给 app queue 时，USB RawHID 使用 MatrixOS app report ID `0xFF`。DeveloperApp 收到的是 report ID 之后的 32-byte app payload。

Request:

```text
[version, seq, command, length, payload...]
```

- `version = 0x01`
- `seq` 是 host 自增序号，reply 原样带回。
- `length` 是 payload byte 数量，不包含 `version, seq, command, length`。
- 未使用 byte 填 `0`。

### HID Host To Device

| Command | Name | Payload |
| --- | --- | --- |
| `0x00` | `PING` | 无 |
| `0x01` | `SET_INPUT_REPORT` | `type, enable` |
| `0x02` | `EXIT_APP` | 无 |
| `0x10` | `LED_WRITE_INDEX` | `flags, entry` |
| `0x11` | `LED_WRITE_XY` | `flags, entry` |
| `0x18` | `LED_WRITE_INDEX_BULK` | `flags, count, entries...` |
| `0x19` | `LED_WRITE_XY_BULK` | `flags, count, entries...` |
| `0x20` | `LED_CANVAS_UPDATE` | 无 |
| `0x21` | `LED_CLEAR_SCREEN` | `[flags]` |
| `0x30` | `KEY_READ_INDEX` | `index_hi, index_lo` |
| `0x31` | `KEY_READ_XY` | `x_i8, y_i8` |

说明：

- `LED_WRITE_*` 的行为由 `flags` 决定。
- `LED_WRITE_INDEX` 和 `LED_WRITE_XY` 只写一个 entry。
- `LED_WRITE_INDEX_BULK` 和 `LED_WRITE_XY_BULK` 按 `count` 写多个 entries。
- `LED_CANVAS_UPDATE` 把 canvas flush 到 LED。
- `LED_CLEAR_SCREEN` 只清空整个屏幕。`flags` 可省略，省略时按 `flags = 0` 处理。
- 清除单个 LED 时，用 `LED_WRITE_*` 写入 RGB `0, 0, 0`。
- HID RGB index entry: `[index_hi, index_lo, r, g, b]`
- HID RGBW index entry: `[index_hi, index_lo, r, g, b, w]`
- HID RGB xy entry: `[x_i8, y_i8, r, g, b]`
- HID RGBW xy entry: `[x_i8, y_i8, r, g, b, w]`

### HID Device To Host

| Command | Name | Payload |
| --- | --- | --- |
| `0x80` | `ACK` | `ack_command, status, data...` |
| `0x81` | `ERROR` | `failed_command, error_code` |
| `0x90` | `INPUT_EVENT` | `event_type, event_data...` |

HID input report 默认关闭。Host 需要按键回报时，发送 `SET_INPUT_REPORT` 开启。

HID `ACK` data:

| Acked command | Status | Data |
| --- | --- | --- |
| `PING` | `OK` | `version, width, height, led_count_hi, led_count_lo` |
| `KEY_READ_INDEX` / `KEY_READ_XY` | `OK` | `event_type, event_data...` |

## MIDI Default API

MIDI default API 不依赖 SysEx，方便 DAW 或 MIDI scripting 直接用。

### MIDI LED Write

| MIDI input | LED result |
| --- | --- |
| `Note On ch1, note, velocity` | `note` 对应 LED 写白色，亮度为 `velocity` |
| `Note On ch2..ch16, note, velocity` | `note` 对应 LED 写 channel hue，亮度为 `velocity` |
| `Note Off` | 清除 `note` 对应 LED |
| `Note On velocity 0` | 等同 `Note Off` |
| `Control Change ch1, CC 120` | clear screen |

Channel color:

- ch1：white
- ch2..ch16：15 个平均分布的 hue
- saturation：`1`
- brightness：velocity / pressure

### MIDI Key Report

| Key input | MIDI output |
| --- | --- |
| `Pressed` | `Note On ch1, note=index, velocity=velocity7` |
| `Aftertouch` / pressure change | `Poly Aftertouch ch1, note=index, pressure=pressure7` |
| `Released` | `Note Off ch1, note=index, velocity=release_velocity7` |

第一版只 report 能映射到 LED index 的 key。没有 index 的 input 暂时不通过 MIDI default report。

## MIDI SysEx API

SysEx 用来做和 HID 类似的完整 command API。所有 payload byte 必须是 7-bit。

SysEx 使用 MatrixOS 已有的 manufacturer / family header，不额外发明 DeveloperApp header。DeveloperApp command 直接跟在 MatrixOS header 后面。

Envelope:

```text
F0 00 02 03 4D 58 command payload... F7
```

- `00 02 03`：manufacturer id
- `4D 58`：Matrix family id

### SysEx Host To Device

| Command | Name | Payload |
| --- | --- | --- |
| `0x00` | `PING` | 无 |
| `0x01` | `SET_INPUT_REPORT` | `type, enable` |
| `0x02` | `EXIT_APP` | 无 |
| `0x10` | `LED_WRITE_INDEX` | `flags, entry` |
| `0x11` | `LED_WRITE_XY` | `flags, entry` |
| `0x18` | `LED_WRITE_INDEX_BULK` | `flags, count, entries...` |
| `0x19` | `LED_WRITE_XY_BULK` | `flags, count, entries...` |
| `0x20` | `LED_CANVAS_UPDATE` | 无 |
| `0x21` | `LED_CLEAR_SCREEN` | `[flags]` |
| `0x30` | `KEY_READ_INDEX` | `index_lsb7, index_msb7` |
| `0x31` | `KEY_READ_XY` | `x_i7, y_i7` |

说明：

- `LED_WRITE_*` 的行为由 `flags` 决定。
- `LED_WRITE_INDEX` 和 `LED_WRITE_XY` 只写一个 entry。
- `LED_WRITE_INDEX_BULK` 和 `LED_WRITE_XY_BULK` 按 `count` 写多个 entries。
- `LED_CLEAR_SCREEN` 只清空整个屏幕。`flags` 可省略，省略时按 `flags = 0` 处理。
- 清除单个 LED 时，用 `LED_WRITE_*` 写入 RGB `0, 0, 0`。
- SysEx RGB index entry: `[index_lsb7, index_msb7, r7, g7, b7]`
- SysEx RGBW index entry: `[index_lsb7, index_msb7, r7, g7, b7, w7]`
- SysEx RGB xy entry: `[x_i7, y_i7, r7, g7, b7]`
- SysEx RGBW xy entry: `[x_i7, y_i7, r7, g7, b7, w7]`

### SysEx Device To Host

| Command | Name | Payload |
| --- | --- | --- |
| `0x80` | `ACK` | `ack_command, status, data...` |
| `0x81` | `ERROR` | `failed_command, error_code` |
| `0x90` | `INPUT_EVENT` | `event_type, event_data...` |

SysEx input report 默认关闭。Host 需要按键回报时，发送 `SET_INPUT_REPORT` 开启。

SysEx `ACK` data:

| Acked command | Status | Data |
| --- | --- | --- |
| `PING` | `OK` | `version, width, height, led_count_lsb7, led_count_msb7` |
| `KEY_READ_INDEX` / `KEY_READ_XY` | `OK` | `event_type, event_data...` |

## Error Code

| Code | Name | Meaning |
| --- | --- | --- |
| `0x00` | `OK` | 成功 |
| `0x01` | `BAD_VERSION` | protocol version 不支持 |
| `0x02` | `BAD_LENGTH` | payload 长度错误 |
| `0x03` | `BAD_PAYLOAD` | payload 内容不合法，比如 LED / key 地址无效，或 input report type 不支持 |
| `0x7F` | `UNKNOWN_COMMAND` | 未知 command |

## V1 Scope

第一版先做：

1. DeveloperApp scaffold 和 app registration。
2. HID `PING`、`SET_INPUT_REPORT`、LED write、bulk LED write、clear screen、key report。
3. MIDI realtime `Start` / `Continue` 打开 default key report。
4. MIDI note LED write 和 MIDI key report。
5. SysEx `PING`、`SET_INPUT_REPORT`、LED write、bulk LED write、clear screen、key read / key report。
6. HID / SysEx `EXIT_APP` 退出 app。

暂缓：

- 多 canvas / 多 layer。
- Host-side input filter。
- 非 keypad input report。
