# MicroPython Runtime Phase 2 TODO

> Historical note: this file records the Phase 2 migration checklist and is no
> longer the active source of truth for open Python runtime work. Current status
> and deferred hardening notes live in `docs/micropython-runtime-phase3.md`.

本文档记录 MicroPython 迁移进入 production-ready 阶段前必须完成的任务。Phase 1 的目标是证明 MicroPython 能在 MatrixOS / MystrixSim 中运行；Phase 2 的目标是把它整理成可维护、可扩展、可验证、能忠实承载原生 App 行为的正式 Python runtime。

## 当前问题

Phase 2 开始前，MicroPython binding 基本都堆在：

- `Applications/Python/MicroPythonPort/usermod/matrixos/modmatrixos.cpp`

这个文件当时同时承担了：

- 公共转换工具：`Color` / `Point` / `Dimension` / `InputId` / callback helper。
- Runtime object type：`Timer`。
- MatrixOS module exports：`SYS` / `LED` / `Input` / `ColorEffects` / `NVS` / `Utils` / `UI`。
- UI native wrappers：`UI` / `Button` / `Selector` / `Number` / `Toggle`。
- 顶层 `MatrixOS` module 注册。

这只是 quick port 可以接受的形态，不适合长期维护。当前已经完成第一轮拆分：`matrixos_module.cpp` 只负责顶层 module 注册，`SYS` / `LED` / `Input` / `NVS` / `UI` 等 first-cut subsystem 已经拆到独立 binding 文件；UI component binding 已拆到 `matrixos_ui_components.cpp`，UIUtility wrapper 已拆到 `matrixos_ui_utility.cpp`。后续 Phase 2 工作继续在这个模块化结构上补 API parity 和测试。

相比之下，PikaPython 的 binding 是按模块拆分的：

- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/MatrixOS_LED.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/MatrixOS_Input.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/MatrixOS_MIDI.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/MatrixOS_HID.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/MatrixOS_USB.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/MatrixOS_NVS.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/MatrixOS_SYS.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/Framework/*.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/UI/*.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/UI/Components/*.cpp`

MicroPython Phase 2 应该采用类似的模块边界，但 public API 不需要保留 Pika 的 PascalCase / native stub 风格。目标是 Pythonic API，底层实现按 MatrixOS subsystem 拆干净。

## Phase 2 验收标准

- [x] `modmatrixos.cpp` 只负责顶层 module 组装和注册，不再承载具体 subsystem 实现。
- [x] 每个 first-cut MatrixOS subsystem 有独立 MicroPython binding 文件和清晰 header。
- [x] Python public API 以 Pythonic endpoint 为准，不保留 Pika 兼容 API。
  - `check_micropython_api_surface.py` 静态扫描 examples/docs。
  - `check_micropython_api_surface.py` 从 `api_introspection.py` 自动提取 `MatrixOS.*` symbol，并要求 `Applications/Python/micropython-api.md` 覆盖。
  - `api_introspection.py` 在真实 MicroPython runtime 中检查 public module、nested module 和常用 public object instance 没有 `Get*` / `Set*` / 旧 Pika wrapper。
- [ ] UI wrapper 足够承载原生 C++ app 的 setting UI 行为。
- [x] MIDI / USB / HID / FileSystem 缺口被实现或明确标记为 unsupported；Logging wrapper、script exception traceback、UI callback exception reporting 已接入 smoke；`print` 与 structured logging 的关系仍作为产品策略保留。
- [ ] Pixel Art / SameGame / Gomoku / Dice 的 Python 版本与原版行为对齐，而不是只验证能启动。
- [x] WebUI RPC 测试覆盖启动、输入、UI callback、FN 短按/长按、NVS、文件、MIDI/HID 可测路径。
- [x] MystrixSim wasm build、WebUI production build、RPC smoke、app parity tests 全部通过。
  - `npm --prefix Devices\MystrixSim\WebUI run verify:micropython` 通过 static checks、MatrixOSHost build、wasm validation、runtime package、WebUI production build。
  - `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev` 通过 core/filesystem/ui/examples RPC smoke。
  - `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-build --skip-web-build` 通过 static checks 和 qstr order check。
  - `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite examples` 通过四个 example 的集中 interaction/startup smoke。
  - UI component binding 拆分后，`npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-web-build --smoke-dev --suite ui` 和 `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite examples` 均通过。
  - Static checks 已覆盖 `package.json` smoke/verify scripts、`micropython-smoke.mjs`、四个 app-specific smoke wrapper 和 `verify-micropython.mjs` 自身，并检查 examples manifest / wrapper / npm script 映射一致。
  - Static checks 已覆盖 MicroPython usermod source manifest：`micropython.mk` 和 `Applications/Python/CMakeLists.txt` 必须显式列出每个 `matrixos_*.cpp`，并禁止旧 `modmatrixos.cpp` 回流。
  - UI component binding 拆分后，`npm --prefix Devices\MystrixSim\WebUI run verify:micropython` 通过 static checks、MatrixOSHost build、wasm validation、runtime package、WebUI production build。
  - UI component binding 拆分后，`npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev` 通过 core、filesystem、ui、examples 全量 RPC smoke。

## Deliverable 1: MicroPython Binding 模块化

**目标**

把 `modmatrixos.cpp` 拆成可维护的 usermod 结构。

**建议目录**

```text
Applications/Python/MicroPythonPort/usermod/matrixos/
  micropython.mk
  matrixos_module.cpp
  matrixos_common.h
  matrixos_common.cpp
  matrixos_timer.cpp
  matrixos_color.cpp
  matrixos_sys.cpp
  matrixos_led.cpp
  matrixos_input.cpp
  matrixos_color_effects.cpp
  matrixos_nvs.cpp
  matrixos_utils.cpp
  matrixos_ui.cpp
  matrixos_ui_components.cpp
  matrixos_ui_utility.cpp
  matrixos_midi.cpp
  matrixos_usb.cpp
  matrixos_hid.cpp
  matrixos_filesystem.cpp
  matrixos_logging.cpp
```

**实现要求**

- `matrixos_common.*` 只放共享转换和 callback helper：
  - `object_to_color`
  - `object_to_point`
  - `object_to_dimension`
  - `object_to_input_id`
  - `make_point`
  - `make_input_id`
  - `make_input_event`
  - protected callback call helper
- 每个 subsystem 文件只导出自己的 `mp_obj_module_t` 或 type declarations。
- `matrixos_module.cpp` 只负责：
  - include 各 subsystem declaration。
  - 组装 `MatrixOS` 顶层 globals。
  - `MP_REGISTER_MODULE(MP_QSTR_MatrixOS, matrixos_module)`。
- `micropython.mk` 改成显式列出所有 `SRC_USERMOD_CXX`，不要只编译一个 `modmatrixos.cpp`。
- 新增文件必须可被 MicroPython qstr generation 正确扫描。
- `verify-micropython.mjs` static gate 必须能发现新增 `matrixos_*.cpp` 但忘记加入 `micropython.mk` 的情况。

**完成标准**

- [x] `modmatrixos.cpp` 被删除或缩减为 `matrixos_module.cpp` 的等价职责。
- [x] First-cut subsystem 已拆分到独立文件；UI component binding 已拆出 `matrixos_ui_components.cpp`，UIUtility 已拆出 `matrixos_ui_utility.cpp`。
- [x] Static gate 会校验 `micropython.mk` 的 `SRC_USERMOD_CXX`、`Applications/Python/CMakeLists.txt` 和 `Applications/Python/MicroPythonPort/usermod/matrixos/matrixos_*.cpp` 一致，防止继续拆分 binding 时漏编译新文件。
- [x] `cmake --build build\MystrixSim --target MatrixOSHost --parallel` 通过。
- [x] `make -f micropython_embed.mk` 能重新生成 embed package。

## Deliverable 2: Public API 设计冻结

**目标**

明确 MicroPython 的 public API，不再边写 example 边猜接口。

**实现要求**

- 写一份 `Applications/Python/micropython-api.md`。
- 每个 module 都要列：
  - public methods/classes/constants。
  - 参数类型。
  - 返回类型。
  - 错误行为。
  - 与 C++ MatrixOS API 的映射关系。
- 保持 Pythonic 命名：
  - 使用 `snake_case`。
  - 不暴露 `GetPosition` / `SetColor` 这种 PascalCase native endpoint。
  - 不保留 Pika compatibility wrapper。
- 明确对象策略：
  - `Color` 是 packed RGB int 还是 value object。
  - `Point` / `Dimension` 是 tuple 还是轻量 object。
  - `InputId` / `InputEvent` / `KeypadInfo` 是 dict、tuple、named object 还是 native type。

**完成标准**

- `api_introspection.py` 与文档接口完全一致。
- 所有 example 只使用文档列出的 public API。
- `rg "Get[A-Z]|Set[A-Z]|PascalCasePattern"` 检查 public Python 示例和文档，不应出现 native-style public API。

## Deliverable 3: UI Wrapper 完整化

**目标**

Python app 可以使用原生 MatrixOS UI framework，而不是自己用 LED/Input 手搓 setting overlay。

**当前状态**

已有基础 wrapper：

- `MatrixOS.UI.UI`
- `MatrixOS.UI.Button`
- `MatrixOS.UI.Selector`
- `MatrixOS.UI.Number`
- `MatrixOS.UI.Toggle`

当前已经通过 WebUI RPC smoke 覆盖基础交互、callback exception、FN release/hold 行为，以及四个 example 的第一层 setting UI interaction。UIUtility 已接入基础 wrapper；`text_scroll`、`color_picker`、`number_selector` 都已有独立交互 smoke。更深的原版 parity 仍需要补剩余复杂 setting 子菜单和动画行为验证。

**待实现**

- [x] `MatrixOS.UI.text_scroll(text, color, speed=10, loop=False)`
- [x] `MatrixOS.UI.color_picker(color, shade=True) -> color | None`
- [x] `MatrixOS.UI.number_selector(value, color, name, lower_limit=INT_MIN, upper_limit=INT_MAX) -> int`
- UI component lifecycle：
  - GC root 清晰。
  - attached component 不能被误删。
  - UI close/clear 后 component 行为明确。
- Callback safety：
  - Python exception 不应破坏 native UI state。
  - callback 内调用 `ui.exit()` / `SYS.exit_app()` 行为可预测。
- Function key behavior：
  - 子 UI 收到 FN release 后退出子 UI。
  - FN release 不泄漏到母 UI。
  - app 自己实现 FN hold exit 时不能被 UI wrapper 吃掉。
- TextScroll behavior：
  - Button hold 滚字。
  - Selector hold 滚当前 item name。
  - Number hold 滚 name。

**完成标准**

- 新增 WebUI RPC interaction test：
  - [x] 构造 UI + Button，注入 grid press/release，验证 callback 被调用。
  - [x] Button hold callback 被调用。
  - [x] Button hold 触发 TextScroll 且不会立即退出。
  - [x] Selector release 改值并调用 callback。
  - [x] FN release 退出 setting UI，母 app 不退出。
  - [x] FN hold 按 app policy 退出 app。
- [x] SameGame / Gomoku / Dice setting UI 改用原生 UI wrapper。

## Deliverable 4: LED API Parity

**目标**

Python LED API 能覆盖 native app 常用能力，而不是只支持 set/fill/update。

**待实现**

- [x] brightness：
  - `LED.set_brightness(value)`
  - `LED.next_brightness()`
  - `LED.set_brightness_multiplier(partition, multiplier_milli)`
  - `multiplier_milli` 是整数 milli-ratio，例如 `1000` 表示 `1.0`，避免 public API 依赖 MicroPython float 配置。
- [x] layer：
  - `LED.current_layer()`
  - `LED.create_layer(crossfade=None)`
  - `LED.copy_layer(dest, src)`
  - `LED.destroy_layer(crossfade=None)`
  - `LED.fade(crossfade=None)`
  - `LED.pause_update(paused=True)`
- [x] partition object：
  - `LED.partitions() -> list[Partition]`
  - `LED.get_partition(index_or_name)`
  - partition exposes `name`, `start`, `size`, `type`, `default_multiplier`。
- [x] batch helpers：
  - `LED.set_many_xy(entries, layer=None)`
  - `LED.set_many_index(entries, layer=None)`

**完成标准**

- API introspection 覆盖当前 LED parity methods。
- Pixel Art 不需要绕过 LED API 写私有状态。
- UI wrapper 的 layer/fade 行为与 native UI 一致。

## Deliverable 5: Input API Parity

**目标**

Python app 可以可靠处理 keypad、function key、aftertouch、velocity、cluster、point mapping。

**待实现**

- `Input.get_event(timeout_ms=0) -> InputEvent | None`
- `Input.clear()`
- `Input.function_key() -> InputId`
- `Input.clusters() -> list[InputCluster]`
- `Input.get_cluster(cluster_id) -> InputCluster | None`
- `Input.primary_grid_cluster() -> InputCluster`
- `Input.get_state(input_id) -> InputSnapshot | None`
- `Input.get_inputs_at(point) -> list[InputId]`
- `Input.get_input_at(cluster_id, point) -> InputId | None`
- `Input.try_get_point(input_id) -> tuple[int, int] | None`
- `Input.get_keypad_capabilities(cluster_id) -> KeypadCapabilities | None`

**对象字段要求**

- `InputEvent`：
  - `id`
  - `input_class`
  - `point`
  - `keypad`
- `KeypadInfo`：
  - `state`
  - `pressed`
  - `released`
  - `hold`
  - `aftertouch`
  - `velocity`
  - `pressure`
  - release velocity 必须保留。
- `InputCluster`：
  - `id`
  - `name`
  - `dimension`
  - `position`
  - `input_class`

**完成标准**

- Hardware / RPC input tests 覆盖 press/release/hold/aftertouch/velocity/pressure。
- Function key 不再用 `(0, 0)` 魔法值判断；example 使用 `Input.function_key()` 或 `InputId` 对象。
- Pixel Art side touch / off-grid input 能按 cluster/point 正确处理。

## Deliverable 6: NVS API 完整化

**目标**

Dice 等 app 的设置持久化不需要 workaround。

**已实现 / 待保持**

- 推荐 public API 是 string key + `NVS.get(key, default)` / `NVS.set(key, value)`。
- 用户不需要自己调用 `Utils.string_hash()`，hash 只是底层实现细节。
- `NVS.set()` 根据 Python value 自动存储 bool、int、str、bytes；int 自动选择能容纳 value 的最小 unsigned 宽度。
- `NVS.get()` 根据 default 类型解码；没有 default 时返回 raw `bytes` 或 `None`。
- `NVS.get_size(key) -> int`
- `NVS.delete(key) -> bool`
- `NVS.get_bytes(key, default=None) -> bytes | None`
- `NVS.set_bytes(key, data) -> bool`
- `NVS.get_string(key, default="") -> str`
- `NVS.set_string(key, value) -> bool`
- `get_u8/u16/u32` / `set_u8/u16/u32` 保留为 fixed-layout advanced helpers。

**完成标准**

- Dice settings 全部走 NVS。
- RPC test 启动 app、改 setting、重启 app、验证 setting 保留。

## Deliverable 7: MIDI API

**目标**

恢复 old Pika facade 覆盖的 MIDI 能力，并用 Pythonic API 重新设计。

**参考**

- `Applications/Python/PikaPython/MatrixOS_MIDI.py`
- `Applications/Python/PikaPython/MatrixOS_MidiPacket.py`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/MatrixOS_MIDI.cpp`
- `Applications/Python/PikaPython/pikascript-lib/MatrixOS/Framework/MatrixOS_MidiPacket.cpp`

**已实现**

- [x] `MIDI.get(timeout_ms=0) -> MidiPacket | None`
- [x] `MIDI.send(packet, port=MIDI.PORT_EACH_CLASS, timeout_ms=0) -> bool`
- [x] `MIDI.send_sysex(port, data, include_meta=True) -> bool`
- [x] `MidiPacket` object:
  - packet factories: note on/off, aftertouch, control change, program change, channel pressure, pitch bend, song position/select, MTC quarter frame, tune request, realtime messages.
  - methods for status, port, channel, note, controller, velocity, value, length, data bytes, SysEx checks.
- [x] constants:
  - MIDI status。
  - MIDI port IDs。

**后续增强**

- [x] `is_note_on()` / `is_note_off()` convenience helpers。
- [ ] 如果需要完整 SysEx receive assembly，再在 Python layer 设计 message-level helper；当前 public API 暴露 packet-level get/send。

**完成标准**

- [x] API introspection 覆盖 construction/send/get no-data path。
- [x] WebUI RPC smoke 覆盖 MIDI RX injection 和 `MIDI.get()`。

## Deliverable 8: USB / HID API

**目标**

Python 可以使用 MatrixOS 的 USB/HID 能力，而不是只靠 WebUI。

**USB**

- [x] `USB.connected()`
- [x] `USB.CDC.connected()`
- [x] `USB.CDC.available()`
- [x] `USB.CDC.poll()`
- [x] `USB.CDC.print(text)`
- [x] `USB.CDC.println(text)`
- [x] `USB.CDC.flush()`
- [x] `USB.CDC.read()`
- [x] `USB.CDC.read_bytes(length)`
- [x] `USB.CDC.read_string()`

**HID**

- [x] `HID.init()`
- [x] `HID.reset()`
- [x] `HID.ready()`
- [x] `HID.Keyboard.tap/press/release/release_all`
- [x] `HID.Gamepad.tap/press/release/release_all/button/buttons/x_axis/y_axis/.../dpad`
- [x] `HID.RawHID.get(timeout_ms=0) -> bytes | None`
- [x] `HID.RawHID.send(data) -> bool`
- keycode constants。

**完成标准**

- [x] API introspection 覆盖 USB no-device safe path。
- [x] WebUI RPC smoke 覆盖 CDC RX injection 和 `read_bytes()`。
- [x] API introspection 覆盖 HID no-device safe path。
- [x] HID functions 在 MystrixSim 或 mock 环境下不 crash。
- [x] RawHID data 使用 `bytes`，不是 list/int workaround。
- [x] WebUI RPC smoke 覆盖 RawHID RX injection 和 `RawHID.get()`。

## Deliverable 9: FileSystem / Import / open()

**目标**

MicroPython 可以加载多文件 app，并支持 Python 标准 `open()` 或 MatrixOS filesystem wrapper。

**已实现**

- 轻量 MicroPython port hooks 接 MatrixOS FileSystem：
  - `mp_import_stat()`
  - `mp_reader_new_file()`
  - `mp_builtin_open()`
- 支持 external import：
  - [x] `MICROPY_ENABLE_EXTERNAL_IMPORT`
  - [x] import path 策略。
  - [x] app local directory。
- [x] `open(path, mode)` 可读写 MatrixOS filesystem。
- [x] MystrixSim staged app 支持多个 `.py` 文件，并支持 staged helper module import。
- [x] `MatrixOS.FileSystem` wrapper:
  - [x] `available()`
  - [x] `exists(path)`
  - [x] `mkdir(path)`
  - [x] `remove(path)`
  - [x] `rmdir(path)`
  - [x] `rename(from_path, to_path)`
  - [x] `list_dir(path="/")`
  - [x] `read_bytes(path)` / `write_bytes(path, data)`
  - [x] `read_text(path)` / `write_text(path, text)`
  - `translate_path` / `ensure_app_directory` 保持 binding 内部实现细节，不暴露为 public Python API。

**完成标准**

- [x] WebUI staged app 支持多个 `.py` 文件。
- [x] 一个 smoke test import 自定义 helper module。
- [x] 一个 smoke test 写文件、读回、删除。

## Deliverable 10: Logging / Error Reporting

**目标**

Python exception 和 app log 能清晰定位问题。

**待实现**

- [x] `MatrixOS.Logging.debug/info/warning/error/verbose(tag, message)`
- [x] logging message 按普通字符串处理，`%` 不会被 Python 调用方当作 printf format。
- [x] exception output 包含：
  - file name。
  - line number。
  - traceback。
- [x] WebUI Python panel 保留完整 exception，不吞输出。
- [x] Runtime panic / callback exception 有一致处理策略。

**完成标准**

- [x] RPC test 运行一个抛异常脚本，验证 traceback 包含文件和行。
- [x] UI callback 抛异常时不会卡死整个 UI loop。

## Deliverable 11: Example App Parity

**目标**

四个 Python example 不是 demo，而是证明 Python API 足够复刻真实 app。

**Pixel Art**

- 对齐 `203Null/MatrixOSPixelArtApp`：
  - 初始 picker 状态。
  - 顶行 color picker。
  - grid art 存储与恢复。
  - FN release toggle picker。
  - FN hold exit。
  - side touch / off-grid cluster 行为。

**SameGame**

- 对齐 `triplefox/Matrix-OS-SameGame`：
  - setup / waiting / moving / compacting / ended state。
  - gravity animation。
  - group removal rule。
  - score rule `n * (n - 2)`。
  - board clear double score。
  - score/game-over TextScroll。
  - settings UI：reset confirm、color count selector。
  - FN release exit settings，FN hold exit app。

**Gomoku**

- 对齐 `sihyunlts/Matrix-OS-Gomoku`：
  - release 落子。
  - current player pulse。
  - last placed flash。
  - win animation。
  - ended reset flow。
  - settings UI：player colors、first player、winning length、reset confirm。
  - FN release exit settings，FN hold exit app。

**Dice**

- 对齐 `Applications/Dice`：
  - rolling/confirmed phases。
  - dot/number mode。
  - dot faces 2-9。
  - number faces 1-99。
  - underglow static/off/breath/strobe/saw。
  - rainbow toggles / periods。
  - settings UI 与 NVS 保存。
  - FN hold settings，FN release roll。

**完成标准**

- 每个 app 有 RPC interaction test，至少覆盖：
  - 启动。
  - 一个主要 gameplay action。
  - 打开 settings。
  - settings 内至少一次修改。
  - 退出 settings。
  - app exit。
- 每个 app 与原版差异必须写在代码注释或文档里，不能隐形简化。

**当前测试进度**

- [x] Pixel Art：RPC smoke 覆盖初始 picker color row、选择颜色、FN release 隐藏 picker、grid paint、left/right touchbar side paint、NVS 10x8 art 存储与重启恢复、FN hold 退出 app。
- [x] SameGame：RPC smoke 覆盖随机棋盘填充、相邻同色 group removal、gravity/compact 后回到稳定可玩棋盘、FN short press/release 进入原生 settings UI、修改 color count 并写入 NVS、settings 内 FN short press/release 返回游戏、reset confirm 子 UI 确认后重建完整棋盘、FN hold 退出 app。
- [x] Gomoku：RPC smoke 覆盖 release 落子、FN press 进入原生 settings UI、reset confirm 子 UI 清空棋盘、修改 player colors、winning length 和 first player 并写入 NVS、settings 内 FN short press/release 返回游戏、settings 内 FN hold 退出 app、三连胜利动画、ended-state input reset flow。
- [x] Dice：RPC smoke 覆盖 dice face render、roll input、FN hold 进入 settings、dot/number mode 切换并写入 NVS、rolling/confirmed rainbow toggle 写入 NVS、dot faces selector 和 number faces selector 子 UI 写入 NVS、rolling/confirmed underglow effect/speed 子 UI 写入 NVS、FN short release 退出 settings、settings 内 FN hold 退出 app。

## Deliverable 12: 测试体系

**目标**

从“手动点一下好像能跑”升级到可重复验证。

**待实现**

- [x] `api_introspection.py` 分层：
  - core API。
  - UI API。
  - IO API。
  - filesystem API。
  - 当前保持单文件 RPC `runText()` 入口，但内部已按 subsystem section 拆成独立 smoke 函数。
- [x] `Devices/MystrixSim/WebUI/tools/micropython-smoke.mjs` 拆分或参数化：
  - `--suite core`
  - `--suite ui`
  - `--suite examples`
  - `--suite filesystem`
- [x] `examples` suite 支持 `--example pixel_art|same_game|gomoku|dice` 过滤，`verify-micropython.mjs` 会转发该参数，方便单独跑某个 app 的 parity。
- [x] 增加 app-specific RPC tests：
  - `pixel-art-smoke.mjs`
  - `same-game-smoke.mjs`
  - `gomoku-smoke.mjs`
  - `dice-smoke.mjs`
- [x] Static gate 会验证 `package.json` 的 smoke/verify script 映射、`micropython-smoke.mjs` 的 examples manifest、四个 app-specific wrapper 的 `--example` 映射，并对主 smoke、四个 wrapper 和 verification runner 执行 `node --check`。
- [x] Static gate 会验证 MicroPython usermod source manifest，确保 `micropython.mk` 和 native CMake source list 覆盖所有 subsystem source，并阻止旧 monolithic `modmatrixos.cpp` 回流。
- [x] 增加 build verification script：
  - build MatrixOSHost。
  - validate wasm。
  - package runtime。
  - WebUI production build。
  - run RPC smoke。
- [x] WebUI RPC 增加 `input.releaseAll`，用于 app parity tests 之间清理 function/grid/touchbar 输入状态，避免 FN hold exit 的定时事件污染后续 app。
- [x] Generic example startup smoke 在启动前和 stop 前释放模拟输入，避免前一个 interaction test 的 held/released state 污染后续 app cleanup。

**完成标准**

- [x] 一个命令可以跑完 Phase 2 验证：`npm --prefix Devices\MystrixSim\WebUI run verify:micropython`，可用 `--smoke-dev` 自动启动 WebUI dev server + Chrome 跑 RPC smoke。
- [x] RPC smoke 可以按 suite 单独运行，例如 `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite ui`。
- [x] Example parity 可以按 app 单独运行，例如 `npm --prefix Devices\MystrixSim\WebUI run verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite examples --example dice`。
- [x] Example parity 也可以用 app-specific npm script 单独运行，例如先打开 RPC 后执行 `npm --prefix Devices\MystrixSim\WebUI run smoke:micropython:dice -- --ws ws://localhost:4012`。
- [x] CI 或本地脚本输出能明确指出哪个 API / app 行为失败。

## Deliverable 13: 文档和代码质量

**目标**

后续开发者能扩展 MicroPython binding，而不是只能在一个大文件里搜索。

**待实现**

- [x] `Applications/Python/MicroPythonPort/README.md` 更新：
  - build/generate embed package。
  - qstr generation 注意事项。
  - 如何新增 subsystem。
  - callback/GC/lifetime 规则。
- [x] `docs/micropython-runtime-port.md` 更新：
  - native target story。
  - memory budget。
  - filesystem/import story。
- [x] `Applications/Python/micropython-api.md` 新增。
- [x] 每个 binding 文件顶部写一句职责说明。
- [x] `check_micropython_api_surface.py` 同时检查禁止的旧 API 形状、手写 public API 文档覆盖，以及 `api_introspection.py` 实际使用的 `MatrixOS.*` symbol 文档覆盖。

**完成标准**

- 新增一个 API 不需要改无关 subsystem 文件。
- [x] qstr regenerate 流程清楚。
- [x] 所有当前 introspection 覆盖的 public API 都能从文档找到。

## 推荐执行顺序

1. 先做 Deliverable 1：拆 `modmatrixos.cpp`。这是后续所有工作的基础。
2. 做 Deliverable 2：冻结 public API 文档，避免继续边写边变。
3. 做 Deliverable 3：补 UI wrapper 和 UI interaction tests。
4. 做 Deliverable 4-10：按 subsystem 补齐 API parity。
5. 做 Deliverable 11：把四个 example 按原版行为接回 API。
6. 做 Deliverable 12-13：测试和文档收口。

## 历史收尾记录

This section is retained as migration history. It described the state at the
end of the Phase 2 push and should not be read as the current open-task list.
Current status and deferred hardening notes live in
`docs/micropython-runtime-phase3.md`.

- First-cut binding 已经按 subsystem 拆开，且 binding/header 文件已有职责说明；UI component binding 已拆成 `matrixos_ui_components.cpp`，UI utility 已拆成 `matrixos_ui_utility.cpp`。
- UI wrapper 已有基础交互、callback exception、Button TextScroll hold、FN release/hold、color picker、number selector 的 RPC smoke。
- `UIUtility` 基础 wrapper 已接入；`text_scroll`、`color_picker`、`number_selector` 都已有 direct RPC interaction smoke，Dice 也间接覆盖 number selector 子 UI。
- MIDI / USB / HID / FileSystem 基础 wrapper 已实现；MicroPython `open()` / import integration 已接入轻量 port hooks；Logging exception 和 callback exception reporting 已由 RPC smoke 覆盖。
- LED / Input / SYS / NVS 已补到当前 smoke contract。
- Pixel Art、SameGame、Gomoku、Dice 已有 app interaction smoke，且已覆盖真实 FN hold exit；后续 Python examples 又加入 Lighting 和 Reversi。
- 当前已有集中式 app parity smoke 和 app-specific wrapper script，覆盖 examples 主流程、settings、NVS 和 app exit；`examples` suite 已支持 `--example` 单独跑某个 app。
- `verify-micropython.mjs --smoke-dev` 已能自动启动 WebUI dev server、Chrome 和 RPC smoke。
- `Applications/Python/micropython-api.md` 已建立，并由 API surface check 维护。
- `verify-micropython.mjs` 会在未设置 `EM_CACHE` 时使用 `build/emscripten-cache`，避免 Windows 上裸 Emscripten build 写入 `C:\Program Files` cache 失败；裸跑 `cmake --build` 时需要手动设置同样的 `EM_CACHE`。
