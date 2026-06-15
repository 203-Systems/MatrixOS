from __future__ import annotations

import ast
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]

CHECKED_PATHS = [
    ROOT / "Applications" / "Python" / "examples",
    ROOT / "docs" / "micropython-api.md",
]

API_DOC_PATH = ROOT / "docs" / "micropython-api.md"
INTROSPECTION_PATH = ROOT / "Applications" / "Python" / "examples" / "api_introspection.py"

FORBIDDEN_PATTERNS = [
    (re.compile(r"\.Get[A-Z][A-Za-z_]*\b"), "native PascalCase getter"),
    (re.compile(r"\.Set[A-Z][A-Za-z_]*\b"), "native PascalCase setter"),
    (re.compile(r"\.ExecuteAPP\b"), "native ExecuteAPP endpoint"),
    (re.compile(r"\.set_color_xy\b"), "removed LED compatibility alias"),
    (re.compile(r"\.set_color_by_id\b"), "removed LED compatibility alias"),
    (re.compile(r"\.primary_grid\("), "old Pika primary_grid wrapper"),
    (re.compile(r"\.keypad_clusters\("), "old Pika keypad_clusters wrapper"),
]

REQUIRED_DOC_SYMBOLS = [
    "MatrixOS.Color",
    "MatrixOS.Timer",
    "MatrixOS.SYS",
    "millis",
    "micros",
    "sleep_ms",
    "task_yield",
    "exit_app",
    "reboot",
    "bootloader",
    "open_setting",
    "execute_app",
    "version",
    "version_id",
    "MatrixOS.LED",
    "clear",
    "fill",
    "set_xy",
    "set_index",
    "fill_partition",
    "update",
    "next_brightness",
    "set_brightness",
    "set_brightness_multiplier",
    "current_layer",
    "create_layer",
    "copy_layer",
    "destroy_layer",
    "fade",
    "pause_update",
    "count",
    "partitions",
    "get_partition",
    "MatrixOS.Input",
    "get_event",
    "function_key",
    "try_get_point",
    "get_state",
    "clusters",
    "get_cluster",
    "primary_grid_cluster",
    "get_inputs_at",
    "get_input_at",
    "get_keypad_capabilities",
    "STATE_IDLE",
    "STATE_ACTIVATED",
    "STATE_PRESSED",
    "STATE_HOLD",
    "STATE_AFTERTOUCH",
    "STATE_RELEASED",
    "MatrixOS.ColorEffects",
    "rainbow",
    "color_breath",
    "color_strobe",
    "color_saw",
    "MatrixOS.NVS",
    "get(key: str | int, default=None)",
    "set(key: str | int, value: bool | int | str | bytes | bytearray)",
    "get_u8",
    "set_u8",
    "get_u16",
    "set_u16",
    "get_u32",
    "set_u32",
    "get_size",
    "delete",
    "get_bytes",
    "set_bytes",
    "get_string",
    "set_string",
    "MatrixOS.Utils",
    "string_hash",
    "MatrixOS.Logging",
    "error",
    "warning",
    "info",
    "debug",
    "verbose",
    "MatrixOS.FileSystem",
    "available",
    "exists",
    "mkdir",
    "remove",
    "rmdir",
    "rename",
    "list_dir",
    "read_bytes",
    "write_bytes",
    "read_text",
    "write_text",
    "MatrixOS.USB",
    "connected",
    "MatrixOS.USB.CDC",
    "poll",
    "print",
    "println",
    "flush",
    "read",
    "read_string",
    "MatrixOS.MIDI",
    "MidiPacket",
    "send",
    "send_sysex",
    "is_note_on",
    "is_note_off",
    "note_on",
    "note_off",
    "aftertouch",
    "control_change",
    "program_change",
    "channel_pressure",
    "pitch_bend",
    "song_position",
    "song_select",
    "mtc_quarter_frame",
    "tune_request",
    "clock",
    "tick",
    "start",
    "continue_",
    "stop",
    "active_sense",
    "reset",
    "status",
    "set_status",
    "port",
    "set_port",
    "channel",
    "set_channel",
    "note",
    "set_note",
    "controller",
    "set_controller",
    "velocity",
    "set_velocity",
    "value",
    "set_value",
    "length",
    "is_sysex",
    "is_sysex_start",
    "data",
    "MatrixOS.HID",
    "MatrixOS.HID.Keyboard",
    "tap",
    "press",
    "release",
    "release_all",
    "MatrixOS.HID.Gamepad",
    "button",
    "buttons",
    "x_axis",
    "y_axis",
    "z_axis",
    "rx_axis",
    "ry_axis",
    "rz_axis",
    "dpad",
    "MatrixOS.HID.RawHID",
    "get",
    "MatrixOS.UI",
    "text_scroll",
    "color_picker",
    "number_selector",
    "UI.UI",
    "set_name",
    "set_color",
    "set_new_layer",
    "allow_exit",
    "set_fps",
    "add",
    "set_setup_func",
    "set_loop_func",
    "set_global_loop_func",
    "set_pre_render_func",
    "set_post_render_func",
    "set_end_func",
    "set_input_handler",
    "Button",
    "Selector",
    "Number",
    "Toggle",
    "set_enabled",
    "set_enable_func",
    "on_press",
    "on_hold",
    "on_change",
]

REQUIRED_DOC_SNIPPETS = [
    "## API 稳定级别",
    "## 错误行为",
    "| `MatrixOS.Color` | `stable` |",
    "| `MatrixOS.Timer` | `stable` |",
    "| `MatrixOS.SYS` | `stable` |",
    "| `MatrixOS.LED` | `stable` |",
    "| `MatrixOS.Input` | `stable` |",
    "| `MatrixOS.ColorEffects` | `stable` |",
    "| `MatrixOS.NVS` | `stable` |",
    "| `MatrixOS.Utils` | `stable` |",
    "| `MatrixOS.Logging` | `stable` |",
    "| `MatrixOS.FileSystem` | `stable` |",
    "| `MatrixOS.USB` | `stable` |",
    "| `MatrixOS.HID` | `stable` |",
    "| `MatrixOS.MIDI` | `stable` |",
    "| `MatrixOS.UI` | `stable` |",
    "NVS `get()` 在 key 不存在时返回传入的 default",
    "UI callback exception 会打印 traceback",
]


def iter_files(path: Path):
    if path.is_file():
        yield path
        return
    for file_path in path.rglob("*"):
        if file_path.suffix in {".py", ".md"}:
            yield file_path


def attribute_chain(node: ast.AST) -> tuple[str, ...] | None:
    parts: list[str] = []
    current = node
    while isinstance(current, ast.Attribute):
        parts.append(current.attr)
        current = current.value

    if isinstance(current, ast.Name):
        parts.append(current.id)
    else:
        return None

    parts.reverse()
    return tuple(parts)


def matrixos_symbols_from_introspection() -> set[str]:
    tree = ast.parse(INTROSPECTION_PATH.read_text(encoding="utf-8"), filename=str(INTROSPECTION_PATH))
    symbols: set[str] = set()

    for node in ast.walk(tree):
        if not isinstance(node, ast.Attribute):
            continue
        chain = attribute_chain(node)
        if chain is None or len(chain) < 2 or chain[0] != "MatrixOS":
            continue
        symbols.add(".".join(chain))

    return symbols


def doc_mentions_symbol(doc_text: str, symbol: str) -> bool:
    if symbol in doc_text:
        return True
    tail = symbol.rsplit(".", 1)[-1]
    return (
        f"`{tail}`" in doc_text
        or f"`{tail}(" in doc_text
        or f"- `{tail}" in doc_text
        or f"### `{tail}" in doc_text
    )


def main() -> int:
    failures: list[str] = []
    for checked_path in CHECKED_PATHS:
        for file_path in iter_files(checked_path):
            text = file_path.read_text(encoding="utf-8")
            for line_number, line in enumerate(text.splitlines(), start=1):
                for pattern, reason in FORBIDDEN_PATTERNS:
                    if pattern.search(line):
                        relative = file_path.relative_to(ROOT)
                        failures.append(f"{relative}:{line_number}: {reason}: {line.strip()}")

    doc_text = API_DOC_PATH.read_text(encoding="utf-8")
    for symbol in REQUIRED_DOC_SYMBOLS:
        if symbol not in doc_text:
            relative = API_DOC_PATH.relative_to(ROOT)
            failures.append(f"{relative}: missing public API documentation for `{symbol}`")

    for snippet in REQUIRED_DOC_SNIPPETS:
        if snippet not in doc_text:
            relative = API_DOC_PATH.relative_to(ROOT)
            failures.append(f"{relative}: missing required API contract text `{snippet}`")

    for symbol in sorted(matrixos_symbols_from_introspection()):
        if not doc_mentions_symbol(doc_text, symbol):
            relative = API_DOC_PATH.relative_to(ROOT)
            source = INTROSPECTION_PATH.relative_to(ROOT)
            failures.append(f"{relative}: `{symbol}` is used by {source} but is not documented")

    if failures:
        print("MicroPython API surface check failed:")
        for failure in failures:
            print(failure)
        return 1

    print("MicroPython API surface check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
