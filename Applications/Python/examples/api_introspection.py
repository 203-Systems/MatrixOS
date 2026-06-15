import MatrixOS


def expect(condition, message):
    if not condition:
        raise AssertionError(message)


def run_section(name, fn):
    print("section", name)
    return fn()


def expect_pythonic_public_api(module, module_name):
    forbidden_names = (
        "ExecuteAPP",
        "get_partition_by_name",
        "keypad_clusters",
        "partition_count",
        "primary_grid",
        "set_color_by_id",
        "set_color_xy",
    )

    for name in dir(module):
        if name in forbidden_names:
            raise AssertionError(module_name + "." + name + " is a removed compatibility API")
        if name.startswith("Get") or name.startswith("Set"):
            raise AssertionError(module_name + "." + name + " is a native-style public API")


def smoke_public_surface():
    expect_pythonic_public_api(MatrixOS, "MatrixOS")
    for module_name in (
        "SYS",
        "LED",
        "Input",
        "ColorEffects",
        "NVS",
        "Utils",
        "Logging",
        "FileSystem",
        "USB",
        "HID",
        "MIDI",
        "UI",
    ):
        expect_pythonic_public_api(getattr(MatrixOS, module_name), "MatrixOS." + module_name)

    expect_pythonic_public_api(MatrixOS.USB.CDC, "MatrixOS.USB.CDC")
    expect_pythonic_public_api(MatrixOS.HID.Keyboard, "MatrixOS.HID.Keyboard")
    expect_pythonic_public_api(MatrixOS.HID.Gamepad, "MatrixOS.HID.Gamepad")
    expect_pythonic_public_api(MatrixOS.HID.RawHID, "MatrixOS.HID.RawHID")


def smoke_color_sys_timer():
    expect(MatrixOS.Color(1, 2, 3) == 0x010203, "Color(r, g, b)")
    expect(MatrixOS.Color((4, 5, 6)) == 0x040506, "Color((r, g, b))")

    now_ms = MatrixOS.SYS.millis()
    now_us = MatrixOS.SYS.micros()
    expect(isinstance(now_ms, int), "SYS.millis returns int")
    expect(isinstance(now_us, int), "SYS.micros returns int")
    expect(isinstance(MatrixOS.SYS.version(), str), "SYS.version returns str")
    expect(isinstance(MatrixOS.SYS.version_id(), int), "SYS.version_id returns int")
    MatrixOS.SYS.task_yield()
    expect(not hasattr(MatrixOS.SYS, "yield_"), "SYS.yield_ is not public")
    expect(not hasattr(MatrixOS.SYS, "error_handler"), "SYS.error_handler is not public")

    timer = MatrixOS.Timer()
    expect(timer.since_last_tick() >= 0, "Timer.since_last_tick")
    expect(isinstance(timer.tick(0), bool), "Timer.tick returns bool")
    timer.record_current()


def smoke_led():
    led_count = MatrixOS.LED.count()
    expect(led_count > 0, "LED.count")
    partitions = MatrixOS.LED.partitions()
    expect(isinstance(partitions, list), "LED.partitions returns list")
    expect(len(partitions) > 0, "LED.partitions non-empty")

    first_partition = partitions[0]
    expect(isinstance(first_partition, dict), "LED.get_partition returns dict")
    expect("name" in first_partition, "partition name")
    expect("start" in first_partition, "partition start")
    expect("size" in first_partition, "partition size")
    expect("type" in first_partition, "partition type")
    expect("default_multiplier" in first_partition, "partition default multiplier")
    expect(MatrixOS.LED.get_partition(0) == first_partition, "LED.get_partition by index")
    expect(MatrixOS.LED.get_partition(first_partition["name"]) == first_partition, "LED.get_partition by name")

    for removed_api in ("partition_count", "get_partition_by_name"):
        expect(not hasattr(MatrixOS.LED, removed_api), "removed LED partition API is not public")
    for removed_alias in ("set_" + "color_xy", "set_" + "color_by_id"):
        expect(not hasattr(MatrixOS.LED, removed_alias), "removed LED compatibility alias is not public")

    MatrixOS.LED.clear()
    MatrixOS.LED.set_xy(0, 0, (16, 32, 48))
    MatrixOS.LED.set_index(0, 0x102030)
    expect(not hasattr(MatrixOS.LED, "set_many_xy"), "LED.set_many_xy is not public")
    expect(not hasattr(MatrixOS.LED, "set_many_index"), "LED.set_many_index is not public")
    MatrixOS.LED.fill_partition(first_partition["name"], 0)
    expect(
        MatrixOS.LED.set_brightness_multiplier(first_partition["name"], first_partition["default_multiplier"]),
        "LED.set_brightness_multiplier",
    )

    layer = MatrixOS.LED.create_layer(0)
    expect(isinstance(layer, int), "LED.create_layer")
    expect(isinstance(MatrixOS.LED.current_layer(), int), "LED.current_layer")
    MatrixOS.LED.copy_layer(layer, layer)
    MatrixOS.LED.fade(0)
    MatrixOS.LED.pause_update(False)
    MatrixOS.LED.update()
    MatrixOS.LED.destroy_layer(0)
    return first_partition


def smoke_input():
    function_key = MatrixOS.Input.function_key()
    expect(type(function_key) is tuple and len(function_key) == 2, "Input.function_key")
    expect(MatrixOS.Input.try_get_point(function_key) is None, "Function key has no point")
    expect(MatrixOS.Input.try_get_point((1, 0)) == (0, 0), "Primary grid point lookup")
    expect(MatrixOS.Input.get_event() is None, "Input.get_event without input")

    clusters = MatrixOS.Input.clusters()
    expect(isinstance(clusters, list), "Input.clusters returns list")
    expect(len(clusters) > 0, "Input.clusters non-empty")

    primary_cluster = MatrixOS.Input.primary_grid_cluster()
    expect(isinstance(primary_cluster, dict), "Input.primary_grid_cluster returns dict")
    expect("id" in primary_cluster, "primary cluster id")
    expect("name" in primary_cluster, "primary cluster name")
    expect("input_class_name" in primary_cluster, "primary cluster class name")
    expect("shape_name" in primary_cluster, "primary cluster shape name")
    expect("dimension" in primary_cluster, "primary cluster dimension")
    expect(MatrixOS.Input.get_cluster(primary_cluster["id"]) is not None, "Input.get_cluster by id")
    expect(MatrixOS.Input.get_cluster(primary_cluster["name"]) == primary_cluster, "Input.get_cluster by name")
    expect(MatrixOS.Input.get_input_at(primary_cluster["id"], (0, 0)) == (primary_cluster["id"], 0), "Input.get_input_at")
    expect(MatrixOS.Input.get_input_at(primary_cluster["name"], (0, 0)) == (primary_cluster["id"], 0), "Input.get_input_at by name")
    expect(MatrixOS.Input.get_inputs_at((0, 0))[0] == (primary_cluster["id"], 0), "Input.get_inputs_at")

    capabilities = MatrixOS.Input.get_keypad_capabilities(primary_cluster["id"])
    expect(
        MatrixOS.Input.get_keypad_capabilities(primary_cluster["name"]) == capabilities,
        "Input.get_keypad_capabilities by name",
    )
    expect(isinstance(capabilities, dict), "Input.get_keypad_capabilities returns dict")
    expect("has_pressure" in capabilities, "keypad capabilities pressure")
    expect("has_aftertouch" in capabilities, "keypad capabilities aftertouch")
    expect("has_velocity" in capabilities, "keypad capabilities velocity")
    expect("has_position" in capabilities, "keypad capabilities position")

    snapshot = MatrixOS.Input.get_state((primary_cluster["id"], 0))
    expect(snapshot is None or "keypad" in snapshot, "Input.get_state keypad snapshot")
    MatrixOS.Input.clear()

    for state in (
        MatrixOS.Input.STATE_IDLE,
        MatrixOS.Input.STATE_ACTIVATED,
        MatrixOS.Input.STATE_PRESSED,
        MatrixOS.Input.STATE_HOLD,
        MatrixOS.Input.STATE_AFTERTOUCH,
        MatrixOS.Input.STATE_RELEASED,
    ):
        expect(isinstance(state, int), "Input state constants")


def smoke_color_effects():
    for color in (
        MatrixOS.ColorEffects.rainbow(),
        MatrixOS.ColorEffects.color_breath(0x204060),
        MatrixOS.ColorEffects.color_strobe((20, 40, 60)),
        MatrixOS.ColorEffects.color_saw((20, 40, 60)),
    ):
        expect(isinstance(color, int), "ColorEffects return int")


def smoke_nvs_utils_logging():
    nvs_key = MatrixOS.Utils.string_hash("MicroPython API smoke")
    expect(isinstance(nvs_key, int), "Utils.string_hash")
    expect(MatrixOS.NVS.set("micropython-api-smoke-generic-int", 17), "NVS.set int")
    expect(MatrixOS.NVS.get("micropython-api-smoke-generic-int", 0) == 17, "NVS.get int")
    expect(MatrixOS.NVS.set("micropython-api-smoke-generic-bool", True), "NVS.set bool")
    expect(MatrixOS.NVS.get("micropython-api-smoke-generic-bool", False) is True, "NVS.get bool")
    expect(MatrixOS.NVS.set("micropython-api-smoke-generic-string", "ok"), "NVS.set string")
    expect(MatrixOS.NVS.get("micropython-api-smoke-generic-string", "") == "ok", "NVS.get string")
    expect(MatrixOS.NVS.set("micropython-api-smoke-generic-bytes", b"abc"), "NVS.set bytes")
    expect(MatrixOS.NVS.get("micropython-api-smoke-generic-bytes") == b"abc", "NVS.get bytes")
    expect(MatrixOS.NVS.set_u8(nvs_key, 17), "NVS.set_u8")
    expect(MatrixOS.NVS.get_u8(nvs_key, 0) == 17, "NVS.get_u8")
    expect(MatrixOS.NVS.set_u16(nvs_key, 0x2345), "NVS.set_u16")
    expect(MatrixOS.NVS.get_u16(nvs_key, 0) == 0x2345, "NVS.get_u16")
    expect(MatrixOS.NVS.set_u32(nvs_key, 0x89ABCDEF), "NVS.set_u32")
    expect(MatrixOS.NVS.get_u32(nvs_key, 0) == 0x89ABCDEF, "NVS.get_u32")
    expect(MatrixOS.NVS.set_string("micropython-api-smoke-string", "ok"), "NVS.set_string str key")
    expect(MatrixOS.NVS.get_string("micropython-api-smoke-string") == "ok", "NVS.get_string str key")
    expect(MatrixOS.NVS.set_bytes("micropython-api-smoke-bytes", b"abc"), "NVS.set_bytes str key")
    expect(MatrixOS.NVS.get_bytes("micropython-api-smoke-bytes") == b"abc", "NVS.get_bytes str key")
    expect(MatrixOS.NVS.get_size("micropython-api-smoke-bytes") == 3, "NVS.get_size")
    expect(MatrixOS.NVS.delete("micropython-api-smoke-bytes"), "NVS.delete")

    MatrixOS.Logging.error("MicroPython", "error smoke with percent % marker")
    MatrixOS.Logging.warning("MicroPython", "warning smoke")
    MatrixOS.Logging.info("MicroPython", "info smoke")
    MatrixOS.Logging.debug("MicroPython", "debug smoke")
    MatrixOS.Logging.verbose("MicroPython", "verbose smoke")


def smoke_filesystem():
    expect(MatrixOS.FileSystem.available(), "FileSystem.available")
    with open("builtin_open_smoke.txt", "w") as f:
        expect(f.write("hello open") == 10, "builtin open write")
    with open("builtin_open_smoke.txt", "r") as f:
        expect(f.read() == "hello open", "builtin open read")

    fs_dir = "api_smoke"
    fs_path = fs_dir + "/sample.txt"
    fs_renamed_path = fs_dir + "/renamed.txt"
    MatrixOS.FileSystem.remove(fs_path)
    MatrixOS.FileSystem.remove(fs_renamed_path)
    MatrixOS.FileSystem.rmdir(fs_dir)
    expect(MatrixOS.FileSystem.mkdir(fs_dir), "FileSystem.mkdir")
    expect(MatrixOS.FileSystem.write_text(fs_path, "hello fs"), "FileSystem.write_text")
    expect(MatrixOS.FileSystem.exists(fs_path), "FileSystem.exists")
    expect(MatrixOS.FileSystem.read_text(fs_path) == "hello fs", "FileSystem.read_text")
    expect(MatrixOS.FileSystem.read_bytes(fs_path) == b"hello fs", "FileSystem.read_bytes")
    expect("sample.txt" in MatrixOS.FileSystem.list_dir(fs_dir), "FileSystem.list_dir")
    expect(MatrixOS.FileSystem.rename(fs_path, fs_renamed_path), "FileSystem.rename")
    expect(MatrixOS.FileSystem.write_bytes(fs_path, b"bytes"), "FileSystem.write_bytes")
    expect(MatrixOS.FileSystem.read_bytes(fs_path) == b"bytes", "FileSystem.write_bytes readback")
    expect(MatrixOS.FileSystem.remove(fs_path), "FileSystem.remove")
    expect(MatrixOS.FileSystem.remove(fs_renamed_path), "FileSystem.remove renamed")
    expect(MatrixOS.FileSystem.rmdir(fs_dir), "FileSystem.rmdir")


def smoke_usb_hid():
    expect(isinstance(MatrixOS.USB.connected(), bool), "USB.connected")
    expect(isinstance(MatrixOS.USB.CDC.connected(), bool), "USB.CDC.connected")
    expect(isinstance(MatrixOS.USB.CDC.available(), int), "USB.CDC.available")
    MatrixOS.USB.CDC.poll()
    MatrixOS.USB.CDC.flush()
    empty_read = MatrixOS.USB.CDC.read()
    expect(empty_read is None or isinstance(empty_read, int), "USB.CDC.read")
    expect(isinstance(MatrixOS.USB.CDC.read_bytes(0), bytes), "USB.CDC.read_bytes")
    expect(isinstance(MatrixOS.USB.CDC.read_string(), str), "USB.CDC.read_string")

    expect(not hasattr(MatrixOS.HID, "init"), "HID.init is not public")
    expect(not hasattr(MatrixOS.HID, "ready"), "HID.ready is not public")
    expect(not hasattr(MatrixOS.HID, "reset"), "HID.reset is not public")
    expect(isinstance(MatrixOS.HID.Keyboard.press(4), bool), "HID.Keyboard.press")
    expect(isinstance(MatrixOS.HID.Keyboard.release(4), bool), "HID.Keyboard.release")
    expect(isinstance(MatrixOS.HID.Keyboard.tap(4, 1), bool), "HID.Keyboard.tap")
    MatrixOS.HID.Keyboard.release_all()
    MatrixOS.HID.Gamepad.press(0)
    MatrixOS.HID.Gamepad.release(0)
    MatrixOS.HID.Gamepad.tap(1, 1)
    MatrixOS.HID.Gamepad.button(2, True)
    MatrixOS.HID.Gamepad.buttons(0)
    MatrixOS.HID.Gamepad.x_axis(123)
    MatrixOS.HID.Gamepad.y_axis(-123)
    MatrixOS.HID.Gamepad.z_axis(0)
    MatrixOS.HID.Gamepad.rx_axis(0)
    MatrixOS.HID.Gamepad.ry_axis(0)
    MatrixOS.HID.Gamepad.rz_axis(0)
    MatrixOS.HID.Gamepad.dpad(0)
    MatrixOS.HID.Gamepad.release_all()
    expect(MatrixOS.HID.RawHID.get(0) is None, "HID.RawHID.get empty")
    expect(isinstance(MatrixOS.HID.RawHID.send(b"abc"), bool), "HID.RawHID.send")


def smoke_midi():
    midi_note = MatrixOS.MIDI.note_on(0, 60, 100)
    expect_pythonic_public_api(midi_note, "MatrixOS.MIDI.MidiPacket")
    expect(isinstance(midi_note, MatrixOS.MIDI.MidiPacket), "MIDI.note_on returns MidiPacket")
    expect(midi_note.status() == MatrixOS.MIDI.STATUS_NOTE_ON, "MidiPacket.status")
    expect(midi_note.channel() == 0, "MidiPacket.channel")
    expect(midi_note.note() == 60, "MidiPacket.note")
    expect(midi_note.velocity() == 100, "MidiPacket.velocity")
    expect(midi_note.value() == 100, "MidiPacket.value")
    expect(midi_note.length() == 3, "MidiPacket.length")
    expect(midi_note.data()[0] == 0x90, "MidiPacket.data status byte")
    expect(not midi_note.is_sysex(), "MidiPacket.is_sysex")
    expect(midi_note.set_channel(1), "MidiPacket.set_channel")
    expect(midi_note.channel() == 1, "MidiPacket.set_channel value")
    expect(midi_note.set_note(61), "MidiPacket.set_note")
    expect(midi_note.note() == 61, "MidiPacket.set_note value")
    expect(midi_note.set_velocity(64), "MidiPacket.set_velocity")
    expect(midi_note.velocity() == 64, "MidiPacket.set_velocity value")
    expect(midi_note.set_status(MatrixOS.MIDI.STATUS_NOTE_OFF), "MidiPacket.set_status")
    expect(midi_note.status() == MatrixOS.MIDI.STATUS_NOTE_OFF, "MidiPacket.set_status value")
    expect(MatrixOS.MIDI.note_off(0, 60, 12).velocity() == 12, "MIDI.note_off release velocity")
    expect(MatrixOS.MIDI.is_note_on(MatrixOS.MIDI.note_on(0, 60, 100)), "MIDI.is_note_on")
    expect(not MatrixOS.MIDI.is_note_on(MatrixOS.MIDI.note_on(0, 60, 0)), "MIDI.is_note_on velocity zero")
    expect(MatrixOS.MIDI.is_note_off(MatrixOS.MIDI.note_off(0, 60, 12)), "MIDI.is_note_off")
    expect(MatrixOS.MIDI.is_note_off(MatrixOS.MIDI.note_on(0, 60, 0)), "MIDI.is_note_off note-on velocity zero")
    expect(MatrixOS.MIDI.aftertouch(0, 60, 42).velocity() == 42, "MIDI.aftertouch")
    expect(MatrixOS.MIDI.control_change(0, 1, 2).controller() == 1, "MIDI.control_change")
    expect(MatrixOS.MIDI.program_change(0, 3).value() == 3, "MIDI.program_change")
    expect(MatrixOS.MIDI.channel_pressure(0, 4).velocity() == 4, "MIDI.channel_pressure")
    expect(MatrixOS.MIDI.pitch_bend(0, 8192).value() == 8192, "MIDI.pitch_bend")
    expect(MatrixOS.MIDI.song_position(12).length() == 3, "MIDI.song_position")
    expect(MatrixOS.MIDI.song_select(2).length() == 2, "MIDI.song_select")
    expect(MatrixOS.MIDI.mtc_quarter_frame(1).length() == 2, "MIDI.mtc_quarter_frame")
    expect(MatrixOS.MIDI.tune_request().status() == MatrixOS.MIDI.STATUS_TUNE_REQUEST, "MIDI.tune_request")
    expect(MatrixOS.MIDI.clock().status() == MatrixOS.MIDI.STATUS_CLOCK, "MIDI.clock")
    expect(MatrixOS.MIDI.tick().status() == MatrixOS.MIDI.STATUS_TICK, "MIDI.tick")
    expect(MatrixOS.MIDI.start().status() == MatrixOS.MIDI.STATUS_START, "MIDI.start")
    expect(MatrixOS.MIDI.continue_().status() == MatrixOS.MIDI.STATUS_CONTINUE, "MIDI.continue_")
    expect(MatrixOS.MIDI.stop().status() == MatrixOS.MIDI.STATUS_STOP, "MIDI.stop")
    expect(MatrixOS.MIDI.active_sense().status() == MatrixOS.MIDI.STATUS_ACTIVE_SENSE, "MIDI.active_sense")
    expect(MatrixOS.MIDI.reset().status() == MatrixOS.MIDI.STATUS_RESET, "MIDI.reset")
    expect(MatrixOS.MIDI.get(0) is None, "MIDI.get empty")
    expect(isinstance(MatrixOS.MIDI.send(midi_note, MatrixOS.MIDI.PORT_OS, 0), bool), "MIDI.send")
    expect(isinstance(MatrixOS.MIDI.send_sysex(MatrixOS.MIDI.PORT_OS, b"abc", False), bool), "MIDI.send_sysex")


def smoke_ui():
    ui = MatrixOS.UI.UI("Smoke UI", MatrixOS.Color(16, 16, 16))
    expect_pythonic_public_api(ui, "MatrixOS.UI.UI")
    button = MatrixOS.UI.Button("Button", MatrixOS.Color(32, 0, 0))
    expect_pythonic_public_api(button, "MatrixOS.UI.Button")
    button.set_size((2, 1))
    button.on_press(lambda: None)
    button.on_hold(lambda: None)
    selector = MatrixOS.UI.Selector((2, 2), 4)
    expect_pythonic_public_api(selector, "MatrixOS.UI.Selector")
    selector.set_value(1)
    expect(selector.get_value() == 1, "UI.Selector value")
    selector.set_color(MatrixOS.Color(0, 32, 0))
    selector.set_lit_mode(MatrixOS.UI.LIT_EQUAL)
    selector.on_change(lambda value: None)
    number = MatrixOS.UI.Number(2)
    expect_pythonic_public_api(number, "MatrixOS.UI.Number")
    number.set_value(42)
    expect(number.get_value() == 42, "UI.Number value")
    number.set_color(MatrixOS.Color(0, 0, 32))
    toggle = MatrixOS.UI.Toggle("Toggle", True)
    expect_pythonic_public_api(toggle, "MatrixOS.UI.Toggle")
    expect(toggle.get_value() is True, "UI.Toggle value")
    toggle.set_color(MatrixOS.Color(32, 32, 0))
    toggle.on_press(lambda value: None)
    expect(hasattr(MatrixOS.UI, "text_scroll"), "UI.text_scroll exists")
    expect(hasattr(MatrixOS.UI, "color_picker"), "UI.color_picker exists")
    expect(hasattr(MatrixOS.UI, "number_selector"), "UI.number_selector exists")
    ui.add(button, (0, 0))
    ui.add(selector, (0, 1))
    ui.add(number, (3, 0))
    ui.add(toggle, (5, 0))
    ui.clear()
    expect(button.close(), "UI.Button close after clear")
    expect(selector.close(), "UI.Selector close after clear")
    expect(number.close(), "UI.Number close after clear")
    expect(toggle.close(), "UI.Toggle close after clear")
    expect(ui.close(), "UI close")


def main():
    print("MatrixOS MicroPython API Introspection")
    run_section("public_surface", smoke_public_surface)
    run_section("color_sys_timer", smoke_color_sys_timer)
    run_section("led", smoke_led)
    run_section("input", smoke_input)
    run_section("color_effects", smoke_color_effects)
    run_section("nvs_utils_logging", smoke_nvs_utils_logging)
    run_section("filesystem", smoke_filesystem)
    run_section("usb_hid", smoke_usb_hid)
    run_section("midi", smoke_midi)
    run_section("ui", smoke_ui)
    print("API introspection ok")


main()
