# MatrixOS MicroPython API

本文档定义 MatrixOS MicroPython runtime 的 public API。目标是 Pythonic、稳定、可测试；不保留 PikaPython 兼容 wrapper，也不暴露 C++ native 的 PascalCase endpoint。

当前 API 仍在 Phase 2 收敛中。本文只把已经进入 MicroPython usermod 并由 `Applications/Python/examples/api_introspection/main.py` 覆盖的接口列为 public。

## API 稳定级别

Phase 3 的目标是冻结 Pythonic public API。当前稳定级别如下：

| Module | Level | 说明 |
| --- | --- | --- |
| `MatrixOS.Color` | `stable` | Packed RGB helper。 |
| `MatrixOS.Timer` | `stable` | 轻量 loop timer。 |
| `MatrixOS.SYS` | `stable` | 系统时间、yield、app lifecycle 和系统动作 wrapper。 |
| `MatrixOS.LED` | `stable` | LED draw、layer、partition 查询。 |
| `MatrixOS.Input` | `stable` | Event、snapshot、cluster 和 keypad capability 查询。 |
| `MatrixOS.ColorEffects` | `stable` | 常用原生颜色效果。 |
| `MatrixOS.NVS` | `stable` | 推荐 string key + `NVS.get/set`；typed helpers 是 advanced fixed-layout API。 |
| `MatrixOS.Utils` | `stable` | 低层工具；普通 NVS 用户不需要直接用 `string_hash()`。 |
| `MatrixOS.Logging` | `stable` | MatrixOS structured logging wrapper。 |
| `MatrixOS.FileSystem` | `stable` | MatrixOS filesystem wrapper，加 lightweight `open()` / import integration。 |
| `MatrixOS.USB` | `stable` | USB / CDC wrapper；无设备时走 safe no-device path。 |
| `MatrixOS.HID` | `stable` | Keyboard、Gamepad、RawHID wrapper；无设备时走 safe no-device path。 |
| `MatrixOS.MIDI` | `stable` | Pythonic `MidiPacket` object 和 MIDI send/receive/factory helpers。 |
| `MatrixOS.UI` | `stable` | 常用 MatrixOS UI component / utility wrapper；复杂交互 parity 继续由 Phase 3 smoke 加强。 |

## 命名和数据约定

- Module 名保持 `MatrixOS`。
- Public method 使用 `snake_case`。
- C++ native API 名称如 `GetPosition`、`SetColor`、`ExecuteAPP` 只能出现在 binding 内部，不能出现在 Python example 或 public docs 里。
- `Color` 是 packed RGB int：`0xRRGGBB`。构造时可以传 `Color(r, g, b)`、`Color(r, g, b, w)`、`Color((r, g, b))` 或 `Color((r, g, b, w))`。当前 public 返回值只承诺 RGB int。
- `Point` / `Dimension` 使用 tuple：`(x, y)`。
- `InputId` 使用 tuple：`(cluster_id, member_id)`。
- NVS key 推荐使用 `str`。底层会映射到 MatrixOS `StringHash`；`int` key 只作为 advanced path 保留。
- `crossfade`、`period`、`offset` 等时间参数单位是 ms。

## 错误行为

- 参数类型错误会抛 MicroPython `TypeError` 或 `ValueError`；错误信息是否带文本取决于当前 MicroPython build 的 error text 配置。
- 查询类 API 在目标不存在时优先返回 `None` 或 `False`，例如 `Input.get_cluster()`、`LED.get_partition()`、`FileSystem.read_text()`。
- 无设备 / 无 host bridge 的 USB、HID、MIDI 路径必须是 safe no-device path：返回 `False`、`None`、`0` 或空 bytes，而不是 crash。
- 会改变 runtime lifecycle 的 API，例如 `SYS.exit_app()`、`SYS.reboot()`、`SYS.bootloader()`、`SYS.execute_app()`，执行成功后当前 Python app 可能不再继续运行。
- UI callback exception 会打印 traceback，并由 WebUI / log 可见；callback exception 不应该破坏 MatrixOS runtime 状态。
- NVS `get()` 在 key 不存在时返回传入的 default；没有 default 时返回 `None`。`set()` 遇到不支持的 value 类型会抛 `TypeError`。

## 顶层

### `MatrixOS.Color(...) -> int`

把 RGB/RGBW 参数转换为 packed RGB int。

```python
MatrixOS.Color(255, 0, 0)
MatrixOS.Color((255, 0, 0))
```

### `MatrixOS.Timer()`

轻量 timer object。

- `timer.tick(ms: int, first_tick: bool = True) -> bool`
- `timer.is_longer(ms: int) -> bool`
- `timer.since_last_tick() -> int`
- `timer.record_current() -> None`

## `MatrixOS.SYS`

- `millis() -> int`
- `micros() -> int`
- `sleep_ms(ms: int) -> None`
- `task_yield() -> None`
- `exit_app() -> None`
- `reboot() -> None`
- `bootloader() -> None`
- `open_setting() -> None`
- `execute_app(app_id: int, args: list[str] = []) -> None`
- `execute_app(author: str, app_name: str, args: list[str] = []) -> None`
- `version() -> str`
- `version_id() -> int`

`task_yield()` 是 cooperative yield 入口，底层走 MatrixOS/FreeRTOS task yield，不引入固定 ms delay。app 不应该依赖它产生精确时间等待；需要等待时间时使用 `sleep_ms()`。Python runtime 会在每次 app `loop()` 返回后自动 yield；普通 app loop 不需要手动调用 `task_yield()`。

`reboot()`、`bootloader()`、`execute_app()` 是真实系统动作。测试里不要直接调用这些会改变 runtime 生命周期的 API，除非测试目标就是系统跳转。

## `MatrixOS.LED`

### 基础绘制

- `clear(layer: int = 255) -> None`
- `fill(color: ColorLike, layer: int = 255) -> None`
- `set_xy(x: int, y: int, color: ColorLike, layer: int = 255) -> None`
- `set_index(index: int, color: ColorLike, layer: int = 255) -> None`
- `fill_partition(name: str, color: ColorLike, layer: int = 255) -> bool`
- `update(layer: int = 255) -> None`

`ColorLike` 可以是 packed int 或 RGB/RGBW tuple。
`set_xy()` 使用 signed XY；Mystrix 系列的 perimeter LED 可以用 `x=-1`、`x=8`、`y=-1`、`y=8` 访问。

### 亮度和 layer

- `next_brightness() -> None`
- `set_brightness(value: int) -> None`
- `set_brightness_multiplier(partition: str, multiplier_milli: int) -> bool`
- `current_layer() -> int`
- `create_layer(crossfade: int = default) -> int`
- `copy_layer(dest: int, src: int) -> None`
- `destroy_layer(crossfade: int = default) -> bool`
- `fade(crossfade: int = default) -> None`
- `pause_update(paused: bool = True) -> None`

`set_brightness_multiplier()` 使用整数 milli-ratio：`1000` 表示 `1.0`，`500` 表示 `0.5`。这样 public API 不依赖 MicroPython float 配置。

### Partition 查询

- `count() -> int`
- `partitions() -> list[dict]`
- `get_partition(index_or_name: int | str) -> dict | None`

Partition dict 字段：

- `name: str`
- `start: int`
- `size: int`
- `type: int`
- `default_multiplier: int`

`default_multiplier` 同样是 milli-ratio。

## `MatrixOS.Input`

### Event 和 state

- `get_event(timeout_ms: int = 0) -> dict | None`
- `clear() -> None`
- `function_key() -> InputId`
- `try_get_point(input_id: InputId) -> Point | None`
- `get_state(input_id: InputId) -> dict | None`

Input event / snapshot dict 字段：

- `id: InputId`
- `input_class: int`
- `point: Point | None`
- `keypad: dict`，仅 keypad input 存在

Keypad dict 字段：

- `state: int`
- `pressed: bool`
- `released: bool`
- `hold: bool`
- `aftertouch: bool`
- `pressure: int`
- `velocity: int`

`velocity` 对 press 和 release 都必须保留，不把 release velocity 固定成 0。

### Cluster 查询

- `clusters() -> list[dict]`
- `get_cluster(cluster: int | str) -> dict | None`
- `primary_grid_cluster() -> dict | None`
- `get_inputs_at(point: Point) -> list[InputId]`
- `get_input_at(cluster: int | str, point: Point) -> InputId | None`
- `get_keypad_capabilities(cluster: int | str) -> dict | None`

Cluster lookup accepts either `id` or `name`, matching LED partition lookup. Prefer names in app code when the device family gives a stable semantic name; use ids when handling raw `InputId` tuples.

Cluster dict 字段：

- `id: int`
- `name: str`
- `input_class: int`
- `input_class_name: str`
- `shape: int`
- `shape_name: str`
- `root_point: Point | None`
- `dimension: Point`
- `input_count: int`
- `has_coordinates: bool`

Keypad capabilities dict 字段：

- `has_pressure: bool`
- `has_aftertouch: bool`
- `has_velocity: bool`
- `has_position: bool`

State 常量：

- `STATE_IDLE`
- `STATE_ACTIVATED`
- `STATE_PRESSED`
- `STATE_HOLD`
- `STATE_AFTERTOUCH`
- `STATE_RELEASED`

## `MatrixOS.ColorEffects`

- `rainbow(period: int = 1000, offset: int = 0) -> int`
- `color_breath(color: ColorLike, period: int = 1000, offset: int = 0) -> int`
- `color_strobe(color: ColorLike, period: int = 1000, offset: int = 0) -> int`
- `color_saw(color: ColorLike, period: int = 1000, offset: int = 0) -> int`

`period` 必须大于 `0`，否则抛 `ValueError`。这避免底层颜色效果执行 modulo-by-zero。

## `MatrixOS.NVS`

推荐入口：

- `get(key: str | int, default=None) -> bool | int | str | bytes | None`
- `set(key: str | int, value: bool | int | str | bytes | bytearray) -> bool`

普通 Python app 应该用 string key，不需要自己调用 `Utils.string_hash()`：

```python
mode = MatrixOS.NVS.get("Python Dice mode", 0)
MatrixOS.NVS.set("Python Dice mode", mode)
```

`get()` 没有 default 时，已有数据按 raw `bytes` 返回；没有数据返回 `None`。传入 default 时，binding 按 default 类型解码：

- `bool` default 返回 `bool`
- `int` default 返回 unsigned integer
- `str` default 返回 `str`
- `bytes` / `bytearray` default 返回 `bytes`

`set()` 根据 Python value 自动选择存储格式：

- `bool` 存 u8
- `int` 自动选择 u8 / u16 / u32 中能容纳 value 的最小 unsigned 宽度；负数会抛 `ValueError`
- `str` 存 UTF-8 bytes
- `bytes` / `bytearray` 存 raw bytes

Advanced fixed-layout API：

- `get_u8(key: str | int, default: int) -> int`
- `set_u8(key: str | int, value: int) -> bool`
- `get_u16(key: str | int, default: int) -> int`
- `set_u16(key: str | int, value: int) -> bool`
- `get_u32(key: str | int, default: int) -> int`
- `set_u32(key: str | int, value: int) -> bool`
- `get_size(key: str | int) -> int`
- `delete(key: str | int) -> bool`
- `get_bytes(key: str | int, default: bytes | None = None) -> bytes | None`
- `set_bytes(key: str | int, data: bytes) -> bool`
- `get_string(key: str | int, default: str = "") -> str`
- `set_string(key: str | int, value: str) -> bool`

## `MatrixOS.Utils`

- `string_hash(text: str) -> int`

## `MatrixOS.Logging`

- `error(tag: str, message: str) -> None`
- `warning(tag: str, message: str) -> None`
- `info(tag: str, message: str) -> None`
- `debug(tag: str, message: str) -> None`
- `verbose(tag: str, message: str) -> None`

`message` 是普通字符串，不是 printf format。即使 message 内包含 `%`，也应该按字面量输出。

## `MatrixOS.FileSystem`

当前 FileSystem public surface 是 MatrixOS filesystem wrapper，走 app sandbox。MicroPython 也支持标准 `open(path, mode)`、external import，以及 MystrixSim WebUI 多文件 staged app。

`open()` 支持常用文本读写模式：`"r"`、`"w"`、`"a"` 和 `"+"` 组合。相对路径会按当前脚本目录解析；WebUI staged app 中会解析到 `host:/python/`，设备脚本会解析到 MatrixOS FileSystem 路径。

External import 使用当前脚本目录加入 `sys.path`，所以 staged app 可以把 `main.py` 和 `helper.py` 一起上传后直接 `import helper`。

- `available() -> bool`
- `exists(path: str) -> bool`
- `mkdir(path: str) -> bool`
- `remove(path: str) -> bool`
- `rmdir(path: str) -> bool`
- `rename(from_path: str, to_path: str) -> bool`
- `list_dir(path: str = "/") -> list[str]`
- `read_bytes(path: str) -> bytes | None`
- `write_bytes(path: str, data: bytes) -> bool`
- `read_text(path: str, encoding: str = "utf-8") -> str | None`
- `write_text(path: str, text: str, encoding: str = "utf-8") -> bool`

`read_*()` 在文件不存在时返回 `None`。`write_*()` 会创建或覆盖文件，并在需要时创建父目录。

## `MatrixOS.USB`

- `connected() -> bool`

### `MatrixOS.USB.CDC`

- `connected() -> bool`
- `available() -> int`
- `poll() -> None`
- `print(text: str) -> None`
- `println(text: str) -> None`
- `flush() -> None`
- `read() -> int | None`
- `read_bytes(length: int) -> bytes`
- `read_string() -> str`

`read()` 在没有数据时返回 `None`。`read_bytes(length)` 最多读取 `length` 个 byte；没有数据时返回 `b""`。`read_string()` 读取当前 CDC RX buffer 中的全部字符串内容。

## `MatrixOS.MIDI`

MIDI public API 使用 Pythonic `MidiPacket` object。所有方法都是 `snake_case`；不暴露旧 Pika / C++ PascalCase endpoint。

- `get(timeout_ms=0) -> MidiPacket | None`
- `send(packet, port=MIDI.PORT_EACH_CLASS, timeout_ms=0) -> bool`
- `send_sysex(port, data, include_meta=True) -> bool`
- `is_note_on(packet) -> bool`
- `is_note_off(packet) -> bool`

### Packet factories

- `note_on(channel, note, velocity) -> MidiPacket`
- `note_off(channel, note, velocity) -> MidiPacket`
- `aftertouch(channel, note, pressure) -> MidiPacket`
- `control_change(channel, controller, value) -> MidiPacket`
- `program_change(channel, program) -> MidiPacket`
- `channel_pressure(channel, pressure) -> MidiPacket`
- `pitch_bend(channel, value) -> MidiPacket`
- `song_position(position) -> MidiPacket`
- `song_select(song) -> MidiPacket`
- `mtc_quarter_frame(value) -> MidiPacket`
- `tune_request() -> MidiPacket`
- `clock() -> MidiPacket`
- `tick() -> MidiPacket`
- `start() -> MidiPacket`
- `continue_() -> MidiPacket`
- `stop() -> MidiPacket`
- `active_sense() -> MidiPacket`
- `reset() -> MidiPacket`

`continue` 是 Python keyword，所以 public factory 名称是 `continue_()`。

`is_note_on()` 只在 status 是 `STATUS_NOTE_ON` 且 velocity 大于 `0` 时返回 `True`。
`is_note_off()` 接受真实 `STATUS_NOTE_OFF`，也接受 MIDI 常见的 `STATUS_NOTE_ON` + velocity `0` 写法。

### `MidiPacket`

- `status() -> int`
- `set_status(status: int) -> bool`
- `port() -> int`
- `set_port(port: int) -> None`
- `channel() -> int | None`
- `set_channel(channel: int) -> bool`
- `note() -> int | None`
- `set_note(note: int) -> bool`
- `controller() -> int | None`
- `set_controller(controller: int) -> bool`
- `velocity() -> int | None`
- `set_velocity(velocity: int) -> bool`
- `value() -> int | None`
- `set_value(value: int) -> bool`
- `length() -> int`
- `is_sysex() -> bool`
- `is_sysex_start() -> bool`
- `data() -> tuple[int, int, int]`

`channel` 是 `0..15`。7-bit MIDI data 是 `0..127`。Pitch bend 和 song position 使用 `0..16383`。

### MIDI constants

Status constants:

- `STATUS_NONE`
- `STATUS_NOTE_OFF`
- `STATUS_NOTE_ON`
- `STATUS_AFTERTOUCH`
- `STATUS_CONTROL_CHANGE`
- `STATUS_PROGRAM_CHANGE`
- `STATUS_CHANNEL_PRESSURE`
- `STATUS_PITCH_BEND`
- `STATUS_MTC_QUARTER_FRAME`
- `STATUS_SONG_POSITION`
- `STATUS_SONG_SELECT`
- `STATUS_TUNE_REQUEST`
- `STATUS_CLOCK`
- `STATUS_TICK`
- `STATUS_START`
- `STATUS_CONTINUE`
- `STATUS_STOP`
- `STATUS_ACTIVE_SENSE`
- `STATUS_RESET`
- `STATUS_SYSEX_DATA`
- `STATUS_SYSEX_END`

Port constants:

- `PORT_EACH_CLASS`
- `PORT_ALL`
- `PORT_USB`
- `PORT_PHYSICAL`
- `PORT_BLUETOOTH`
- `PORT_WIRELESS`
- `PORT_RTP`
- `PORT_DEVICE_CUSTOM`
- `PORT_SYNTH`
- `PORT_OS`
- `PORT_INVALID`

## `MatrixOS.HID`

### `MatrixOS.HID.Keyboard`

- `tap(keycode: int, length_ms: int = 100) -> bool`
- `press(keycode: int) -> bool`
- `release(keycode: int) -> bool`
- `release_all() -> None`

### `MatrixOS.HID.Gamepad`

- `tap(button_id: int, length_ms: int = 100) -> None`
- `press(button_id: int) -> None`
- `release(button_id: int) -> None`
- `release_all() -> None`
- `button(button_id: int, state: bool) -> None`
- `buttons(button_mask: int) -> None`
- `x_axis(value: int) -> None`
- `y_axis(value: int) -> None`
- `z_axis(value: int) -> None`
- `rx_axis(value: int) -> None`
- `ry_axis(value: int) -> None`
- `rz_axis(value: int) -> None`
- `dpad(direction: int) -> None`

Axis value 使用原生 int16 范围：`-32768..32767`。binding 会 clamp 超出范围的值。

### `MatrixOS.HID.RawHID`

- `get(timeout_ms: int = 0) -> bytes | None`
- `send(data: bytes) -> bool`

`RawHID.get()` 没有数据时返回 `None`，收到 report 时返回实际长度的 `bytes`。

## `MatrixOS.UI`

当前 UI wrapper 已冻结基础 component surface 和常用 UI utility，足够承载当前 Python example app 的 setting UI。更长时间的 stress / product polish 记录在 `docs/micropython-runtime-phase3.md`。

### Module 常量

Selector direction：

- `DIRECTION_RIGHT_THEN_DOWN`
- `DIRECTION_DOWN_THEN_RIGHT`
- `DIRECTION_LEFT_THEN_DOWN`
- `DIRECTION_DOWN_THEN_LEFT`
- `DIRECTION_UP_THEN_RIGHT`
- `DIRECTION_RIGHT_THEN_UP`
- `DIRECTION_UP_THEN_LEFT`
- `DIRECTION_LEFT_THEN_UP`

Selector lit mode：

- `LIT_EQUAL`
- `LIT_LESS_EQUAL_THAN`
- `LIT_GREATER_EQUAL_THAN`
- `LIT_ALWAYS`

### Utility functions

- `text_scroll(text: str, color: ColorLike, speed: int = 10, loop: bool = False) -> None`
- `color_picker(color: ColorLike, shade: bool = True) -> int | None`
- `number_selector(value: int, color: ColorLike, name: str, lower_limit: int = INT_MIN, upper_limit: int = INT_MAX) -> int`

这些函数直接映射到原生 `MatrixOS::UIUtility`。`color_picker()` 在用户确认时返回 packed RGB int；用户取消时返回 `None`。

### `UI.UI(name: str = "", color: ColorLike = Color.White, new_layer: bool = True)`

- `start() -> None`
- `exit() -> None`
- `close() -> bool`
- `set_name(name: str) -> None`
- `set_color(color: ColorLike) -> None`
- `set_new_layer(create: bool) -> None`
- `allow_exit(allow: bool) -> None`
- `set_fps(fps: int) -> None`
- `add(component, position: Point) -> None`
- `clear() -> None`
- `set_setup_func(callback) -> None`
- `set_loop_func(callback) -> None`
- `set_global_loop_func(callback) -> None`
- `set_pre_render_func(callback) -> None`
- `set_post_render_func(callback) -> None`
- `set_end_func(callback) -> None`
- `set_input_handler(callback) -> None`

`set_input_handler(callback)` 的 callback 接收 InputEvent dict，返回 truthy 表示 consume event。

### Shared component methods

`Button`、`Selector`、`Number`、`Toggle`、`CustomComponent` 都支持：

- `close() -> bool`
- `set_enabled(enabled: bool) -> None`
- `set_enable_func(callback) -> None`

### `UI.Button(name: str = "", color: ColorLike = Color.White)`

- `set_name(name: str) -> None`
- `set_color(color: ColorLike) -> None`
- `set_size(size: Dimension) -> None`
- `set_color_func(callback) -> None`
- `on_press(callback) -> None`
- `on_hold(callback) -> None`

### `UI.Selector(dimension: Dimension = (1, 1), count: int = 1)`

- `set_value(value: int) -> None`
- `get_value() -> int`
- `set_value_func(callback) -> None`
- `on_change(callback) -> None`
- `set_dimension(dimension: Dimension) -> None`
- `set_name(name: str) -> None`
- `set_count(count: int) -> None`
- `set_direction(direction: int) -> None`
- `set_lit_mode(lit_mode: int) -> None`
- `set_color(color: ColorLike) -> None`
- `set_color_func(callback) -> None`
- `set_individual_color_func(callback) -> None`
- `set_individual_name_func(callback) -> None`

### `UI.Number(digits: int = 1)`

- `set_value(value: int) -> None`
- `get_value() -> int`
- `set_value_func(callback) -> None`
- `set_name(name: str) -> None`
- `set_color(color: ColorLike) -> None`
- `set_alternative_color(color: ColorLike) -> None`
- `set_digits(digits: int) -> None`
- `set_spacing(spacing: int) -> None`
- `set_color_func(callback) -> None`

### `UI.Toggle(name: str = "", value: bool = False)`

- `set_value(value: bool) -> None`
- `get_value() -> bool`
- `set_name(name: str) -> None`
- `set_color(color: ColorLike) -> None`
- `set_color_func(callback) -> None`
- `set_size(size: Dimension) -> None`
- `on_press(callback) -> None`
- `on_hold(callback) -> None`

### `UI.CustomComponent(dimension: Dimension = (1, 1))`

轻量 Python-defined UI component。用于少量 app-specific control；高频复杂绘制仍应优先写 native component。

- `set_size(size: Dimension) -> None`
- `set_render_func(callback) -> None`
- `set_key_func(callback) -> None`

`render` callback 接收 `origin: Point`，在当前 UI layer 上直接用 `MatrixOS.LED` 绘制，返回 truthy 表示已处理 render。UI runtime 会在调用 component render 前清屏。

`key` callback 接收 `xy: Point` 和 `keypad: dict`。`xy` 是相对 component origin 的坐标，`keypad` 字段同 `Input` event 中的 `keypad` dict。返回 truthy 表示 consume event，避免事件继续传给下层 component 或触发 UI text scroll。

## 验证

当前最低验证路径：

```powershell
python Applications\Python\tools\check_micropython_api_surface.py
bash -lc "cd /mnt/c/Users/NengzhuoCai/Documents/GitHub/Matrix/MatrixOS/Applications/Python/MicroPythonPort && make -f micropython_embed.mk"
$env:EM_CACHE='C:\Users\NengzhuoCai\Documents\GitHub\Matrix\MatrixOS\build\emscripten-cache'
cmake --build build\MystrixSim --target MatrixOSHost --parallel
node Devices\MystrixSim\tools\validate-runtime-wasm.mjs build\MystrixSim\Devices\MystrixSim\MatrixOSHost.wasm
node Devices\MystrixSim\WebUI\tools\package-runtime.mjs build\MystrixSim\Devices\MystrixSim\MatrixOSHost.js build\MystrixSim\Devices\MystrixSim\MatrixOSHost.wasm Devices\MystrixSim\WebUI\public\MatrixOS.msfw
npm --prefix Devices\MystrixSim\WebUI run smoke:micropython -- --ws ws://localhost:4012
```

`api_introspection.py` 是本文档的 runtime smoke contract。新增 public API 时必须同步更新本文档和 introspection。
