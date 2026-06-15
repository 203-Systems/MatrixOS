# MicroPython Runtime Phase 3 TODO

本文档记录 MicroPython runtime 从 Phase 2 的 production-ready 迁移工作，进入可以长期维护、面向真实设备发布、持续承载 Python App 的 Phase 3 任务。

Phase 2 的重点是把 runtime 跑起来、拆出模块化 binding、冻结 Pythonic API、补齐主要 MatrixOS subsystem wrapper，并用 WebUI RPC smoke 验证四个 example 的主流程。Phase 3 的重点不是继续堆接口，而是把稳定性、原版行为一致性、设备部署、维护工具和发布体验收束到可以依赖的状态。

## Phase 3 原则

- Python example 和 wrapper 不应该为了绕过 runtime / C++ framework 问题写 workaround。
- 如果 Python 层暴露出 UI、Input、LED、FileSystem 或其他 MatrixOS framework 的设计问题，优先记录并反馈；必要时改进 C++ 侧能力，而不是把复杂补丁塞进 Python app。
- Python API 应该保持清爽、Pythonic、可解释；不为了兼容旧 Pika facade 或某个临时 example 形状保留多余 endpoint。

## Phase 3 验收标准

- [x] `Applications/Python/micropython-api.md` 记录 stable public contract 基础结构，新增 / 删除 / 改名 API 会经过 introspection、文档和 smoke 静态检查。
- [ ] UI wrapper 可以承载常见 MatrixOS setting UI，不需要 example app 自己写 input consume / suppress workaround。
- [x] WebUI RPC smoke 的 examples suite 与 single example 入口稳定可重复；full verify 仍需继续覆盖 production build smoke 和后续 stress suite。
- [ ] MicroPython runtime 可以在目标设备上按 family 配置 heap、stack、script storage、stdout/stdin，并有明确 unsupported feature 策略。
- [ ] MicroPython usermod binding 结构清晰，新增 subsystem / component 不会破坏 qstr generation、CMake、makefile 和文档检查。
- [ ] Python app 开发体验成型：能写、跑、调试、保存、上传、查看 traceback，并能知道当前设备支持哪些 API。

## Deliverable 1: Public API Freeze

**目标**

把当前 Pythonic API 从“实现中接口”变成稳定 contract。

**任务**

- [ ] 清理所有 public-facing PascalCase / Pika compatibility 残留。
- [x] 明确每个 module 的稳定级别：
  - `stable`
  - `experimental`
  - `sim-only`
  - `unsupported`
- [x] `Applications/Python/micropython-api.md` 补齐错误行为：
  - 参数类型错误。
  - 设备能力不存在。
  - runtime inactive。
  - callback exception。
- [x] NVS public API 保持用户友好：推荐 string key + `NVS.get/set`，hash 和固定 size typed API 只作为 advanced path。
- [ ] `api_introspection.py` 覆盖所有 public class、instance method、constant 和 nested module。
- [x] `check_micropython_api_surface.py` 在 verify 中阻止 undocumented public symbol，并要求 API 稳定级别 / 错误行为 contract 文档存在。
- [x] qstr order check 和 generated package check 保持在 `verify:micropython` 默认路径里。

**交付物**

- `Applications/Python/micropython-api.md` 更新为稳定 API 文档。
- `Applications/Python/examples/api_introspection/main.py` 覆盖完整 public surface。
- `Applications/Python/tools/check_micropython_api_surface.py` 能发现文档/API 漂移。

## Deliverable 2: UI Wrapper Production Parity

**目标**

Python App 可以像 C++ App 一样使用 MatrixOS UI，不需要手写底层 input 消费逻辑。

**任务**

- [ ] UI layer push / pop、fade、focus、input consume 行为与 C++ app 对齐。
- [ ] FN release / hold 行为在 parent UI、child UI、TextScroll、ColorPicker、NumberSelector 中一致。
- [ ] `Button` hold TextScroll 稳定。
- [ ] `Selector` hold 当前 item name TextScroll 稳定。
- [ ] `Number` hold name TextScroll 稳定。
- [ ] UI callback exception 不破坏 runtime 状态，traceback 能回到 WebUI / log。
- [ ] UI component callback 生命周期安全：
  - Python callable 不被提前 GC。
  - UI 退出后不调用悬空 callback。
  - callback 内 exit / push child UI 行为稳定。
- [ ] 补齐常用 UI utility wrapper：
  - `text_scroll`
  - `color_picker`
  - `number_selector`
  - 后续如 C++ UI utility 有新增，需要按同样模式补 wrapper。

**交付物**

- `Devices/MystrixSim/WebUI/tools/micropython-smoke.mjs` UI suite 覆盖基础 UI、TextScroll hold、child utility、callback exception、FN short / hold。
- 不需要 app 代码显式调用 `clear_input_buffer()` / `suppress_active_inputs()` 来修 UI 退出问题。

## Deliverable 3: WebUI RPC Test Reliability

**目标**

测试失败时能区分 runtime bug、WebUI bridge bug、输入状态污染和测试脚本问题。

**任务**

- [ ] RPC helper 增加统一 cleanup：
  - release all input。
  - stop active Python app。
  - clear LED frame / staged files / relevant NVS fixture。
- [x] examples smoke case 在 fresh `--smoke-dev` runtime 下独立，不依赖前一个 case 的 state。
- [ ] `--suite core|filesystem|ui|examples` 稳定。
- [x] `--example pixel_art|same_game|gomoku|dice` 入口稳定。
- [x] RPC timeout / browser bridge timeout 输出具体 stage 和最后 runtime status。
- [x] 增加失败时 artifact：
  - bridge status。
  - Python output tail。
  - LED frame snapshot。
  - active input state。
  - active app/runtime status。
  - Python runtime debug / heap summary。
- [ ] WebUI dev smoke 和 production build smoke 都能跑。

**当前验证**

- `verify:micropython -- --skip-build --skip-web-build` 通过 static/API/qstr gate。
- `verify:micropython -- --skip-static --skip-build --skip-web-build --smoke-dev --suite examples` 通过 Pixel Art、SameGame、Gomoku、Dice。
- `verify:micropython` 完整路径通过 MatrixOSHost build、wasm validate、runtime package、WebUI production build。
- `--smoke-dev` 默认使用 fresh free ports，避免 Windows 上旧 dev server / Chrome 子进程污染新测试。

**交付物**

- `npm --prefix Devices\MystrixSim\WebUI run verify:micropython` 作为默认回归入口。
- App-specific smoke wrapper 与 manifest / package script 映射由 static gate 检查。

## Deliverable 4: Runtime Robustness

**目标**

Python crash 不应该拖垮 MatrixOS；Python app 退出后 runtime state 干净。

**任务**

- [ ] 脚本 exception、callback exception、import error、syntax error 都能正确退出或回到 REPL。
- [ ] 长时间 loop 不导致 input queue / LED buffer / callback reference 泄漏。
- [ ] App exit 后：
  - active input 清理。
  - UI layer 清理。
  - pending callback 清理。
  - file handle 清理。
  - stdout subscriber 清理。
- [x] REPL 和 script mode 的基础生命周期隔离由 core smoke 与 lifecycle smoke 覆盖。
- [ ] 内存不足时给出清晰错误，不出现 silent corruption。
- [x] 增加 runtime lifecycle smoke：
  - 多次启动 / 停止。
  - Python-level `ValueError` 后继续运行脚本。
  - 长时间 app 被 `python.stop` 停止后 RPC 仍可响应。
  - active runtime memory summary 可通过 `python.debug` 读取。
- [ ] 扩展 runtime stress smoke：
  - 多次 import。
  - 多次 UI enter / exit。
  - 大量 LED write。

**交付物**

- WebUI smoke 新增 `lifecycle` suite。
- 文档记录当前 heap 下可承载脚本规模。

## Deliverable 5: Device Runtime Readiness

**目标**

MicroPython 不只是 MystrixSim 功能，也能按设备 family 安全启用。

**任务**

- [ ] 为每个目标 family 明确 heap size。
- [ ] 为每个目标 family 明确 stack guard / stack limit。
- [ ] 明确 script storage policy：
  - 从 FileSystem 加载。
  - 从 app args 加载。
  - 从 WebUI / host staging 加载。
- [ ] 明确 stdout / stdin：
  - USB CDC。
  - WebUI console。
  - device log。
- [ ] 设备上验证：
  - REPL 启动 / Ctrl-D 退出。
  - script mode。
  - import sibling module。
  - `open()` 读写。
  - traceback。
  - NVS。
  - LED / Input / UI。
- [ ] 明确 unsupported API 在无对应硬件上的行为。

**交付物**

- `docs/micropython-runtime-port.md` 增加 device-family checklist。
- 至少一个真实设备 family 有通过记录和调参说明。

## Deliverable 6: FileSystem / Import / Packaging

**目标**

Python app 可以自然组织成多文件项目。

**任务**

- [ ] 明确 app package layout：
  - single file。
  - staged multi-file。
  - future packaged app。
- [ ] `sys.path`、relative import、absolute import 行为文档化。
- [ ] `open()` 路径规则文档化：
  - script-relative。
  - app sandbox。
  - host staging。
- [ ] 支持常用文本和 bytes 文件操作。
- [ ] import error traceback 指向正确文件名 / 行号。
- [ ] WebUI Python panel 支持多文件编辑 / staged run 的最终 UX。

**交付物**

- `Applications/Python/micropython-api.md` FileSystem / import 章节更新。
- `filesystem` suite 覆盖多文件 import、relative open、bytes read/write。

## Deliverable 7: Logging / Debugging UX

**目标**

用户能知道 Python app 为什么挂了，也能在 WebUI 里有效调试。

**任务**

- [ ] 决定 `print()` 和 `MatrixOS.Logging` 的产品关系。
- [ ] WebUI Python panel 显示：
  - stdout。
  - stderr / traceback。
  - active app status。
  - last exception。
  - runtime memory summary。
- [ ] callback exception 标出 callback 来源。
- [ ] RPC 增加必要 debug endpoint：
  - [x] runtime status。
  - [x] heap usage。
  - [x] active input state。
  - [x] loaded / staged script file metadata。
- [ ] 文档写清推荐调试方式。

**交付物**

- `docs/micropython-runtime-port.md` 增加 debugging section。
- WebUI smoke 覆盖 traceback 和 callback exception display。

## 当前剩余风险

- UI wrapper 复杂交互仍是最大风险，尤其是 nested UI、TextScroll、FN release/hold consume。
- 如果 Python example 里出现必须靠 app 自己处理的奇怪状态，可能说明 C++ framework 或 binding 还缺一个正确抽象，应该反馈并修底层。
- WebUI RPC smoke 依赖 browser bridge，失败时需要更好的 artifact 才能快速定位。
- 真实设备 heap / stack / storage 策略还没有完全按 family 收束。
- MicroPython public API 一旦被 example 使用，就要避免继续随手改名；后续改动需要 migration 计划。

## 建议执行顺序

1. 清理 API 文档和 introspection，冻结 public surface。
2. 稳定 UI suite，尤其是 Button / Selector / Number TextScroll hold 和 nested UI exit。
3. 提升 WebUI RPC test reliability，让失败 artifact 足够定位问题。
4. 做 runtime robustness：exception、lifecycle、cleanup、stress。
5. 做 device-family readiness，至少选一个真实设备跑通。
6. 完善 FileSystem/import/package 和 WebUI debugging UX。
