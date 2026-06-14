import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))


class NativeMidiPacket:
    def __init__(self, status=0, data1=0, data2=0):
        self.status_value = status
        self.channel_value = 0
        self.note_value = data1
        self.controller_value = data1
        self.velocity_value = data2
        self.value_value = data2

    def NoteOn(self, channel, note, velocity):
        packet = NativeMidiPacket(0x90, note, velocity)
        packet.channel_value = channel
        return packet

    def NoteOff(self, channel, note, velocity):
        packet = NativeMidiPacket(0x80, note, velocity)
        packet.channel_value = channel
        return packet

    def AfterTouch(self, channel, note, pressure):
        packet = NativeMidiPacket(0xA0, note, pressure)
        packet.channel_value = channel
        return packet

    def ControlChange(self, channel, controller, value):
        packet = NativeMidiPacket(0xB0, controller, value)
        packet.channel_value = channel
        return packet

    def ProgramChange(self, channel, program):
        packet = NativeMidiPacket(0xC0, program, 0)
        packet.channel_value = channel
        return packet

    def ChannelPressure(self, channel, pressure):
        packet = NativeMidiPacket(0xD0, pressure, 0)
        packet.channel_value = channel
        return packet

    def PitchBend(self, channel, value):
        packet = NativeMidiPacket(0xE0, 0, value)
        packet.channel_value = channel
        return packet

    def Clock(self):
        return NativeMidiPacket(0xF8)

    def Start(self):
        return NativeMidiPacket(0xFA)

    def Continue(self):
        return NativeMidiPacket(0xFB)

    def Stop(self):
        return NativeMidiPacket(0xFC)

    def ActiveSense(self):
        return NativeMidiPacket(0xFE)

    def Reset(self):
        return NativeMidiPacket(0xFF)

    def SongPosition(self, position):
        return NativeMidiPacket(0xF2, position & 0x7F, (position >> 7) & 0x7F)

    def SongSelect(self, song):
        return NativeMidiPacket(0xF3, song, 0)

    def TuneRequest(self):
        return NativeMidiPacket(0xF6)

    def Port(self):
        return 0x100

    def Status(self):
        return self.status_value

    def Channel(self):
        return self.channel_value

    def Note(self):
        return self.note_value

    def Controller(self):
        return self.controller_value

    def Velocity(self):
        return self.velocity_value

    def Value(self):
        return self.value_value

    def SetStatus(self, status):
        self.status_value = status
        return True

    def SetChannel(self, channel):
        self.channel_value = channel
        return True

    def SetNote(self, note):
        self.note_value = note
        return True

    def SetController(self, controller):
        self.controller_value = controller
        return True

    def SetVelocity(self, velocity):
        self.velocity_value = velocity
        return True

    def SetValue(self, value):
        self.value_value = value
        return True

    def Length(self):
        return 3

    def SysEx(self):
        return self.status_value == 0xF0

    def SysExStart(self):
        return self.status_value == 0xF0


midi_packet_module = types.ModuleType("_MatrixOS_MidiPacket")
midi_packet_module.MidiPacket = NativeMidiPacket
sys.modules["_MatrixOS_MidiPacket"] = midi_packet_module

for module_name in ("MatrixOS_MidiPortID", "MatrixOS_MidiStatus"):
    module = types.ModuleType(module_name)
    module.MidiPortID = object
    module.MidiStatus = object
    sys.modules[module_name] = module

midi_calls = []
midi_module = types.ModuleType("_MatrixOS_MIDI")
midi_module.Get = lambda timeout_ms: None
midi_module.Send = lambda packet, timeout_ms: midi_calls.append(("send", packet, timeout_ms)) or True
midi_module.SendSysEx = lambda port, length, data, include_meta: midi_calls.append(("sysex", port, length, data, include_meta)) or True
sys.modules["_MatrixOS_MIDI"] = midi_module

hid_module = types.ModuleType("_MatrixOS_HID")
hid_module.Ready = lambda: True
sys.modules["_MatrixOS_HID"] = hid_module

hid_calls = []
for native_name in (
    "_MatrixOS_HID_Keyboard",
    "_MatrixOS_HID_Gamepad",
    "_MatrixOS_HID_Mouse",
    "_MatrixOS_HID_Consumer",
    "_MatrixOS_HID_System",
    "_MatrixOS_HID_Touch",
):
    module = types.ModuleType(native_name)
    module.Tap = lambda *args, _name=native_name: hid_calls.append((_name, "Tap", args)) or True
    module.Press = lambda *args, _name=native_name: hid_calls.append((_name, "Press", args)) or True
    module.Release = lambda *args, _name=native_name: hid_calls.append((_name, "Release", args)) or True
    module.ReleaseAll = lambda _name=native_name: hid_calls.append((_name, "ReleaseAll", ()))
    module.Click = lambda *args, _name=native_name: hid_calls.append((_name, "Click", args))
    module.Write = lambda *args, _name=native_name: hid_calls.append((_name, "Write", args))
    module.Button = lambda *args, _name=native_name: hid_calls.append((_name, "Button", args))
    module.Buttons = lambda *args, _name=native_name: hid_calls.append((_name, "Buttons", args))
    module.XAxis = lambda *args, _name=native_name: hid_calls.append((_name, "XAxis", args))
    module.YAxis = lambda *args, _name=native_name: hid_calls.append((_name, "YAxis", args))
    module.ZAxis = lambda *args, _name=native_name: hid_calls.append((_name, "ZAxis", args))
    module.RXAxis = lambda *args, _name=native_name: hid_calls.append((_name, "RXAxis", args))
    module.RYAxis = lambda *args, _name=native_name: hid_calls.append((_name, "RYAxis", args))
    module.RZAxis = lambda *args, _name=native_name: hid_calls.append((_name, "RZAxis", args))
    module.DPad = lambda *args, _name=native_name: hid_calls.append((_name, "DPad", args))
    module.MoveTo = lambda *args, _name=native_name: hid_calls.append((_name, "MoveTo", args))
    module.Move = lambda *args, _name=native_name: hid_calls.append((_name, "Move", args))
    sys.modules[native_name] = module

raw_hid_module = types.ModuleType("_MatrixOS_HID_RawHID")
raw_hid_module.Get = lambda timeout_ms: b"reply"
raw_hid_module.Send = lambda report, length: hid_calls.append(("RawHID", "Send", (report, length))) or True
sys.modules["_MatrixOS_HID_RawHID"] = raw_hid_module

nvs_store = {}
nvs_module = types.ModuleType("_MatrixOS_NVS")
nvs_module.GetSize = lambda key: len(nvs_store.get(key, b""))
nvs_module.GetVariable = lambda key: nvs_store.get(key, None)
nvs_module.SetVariable = lambda key, data, length: nvs_store.__setitem__(key, data[:length]) or True
nvs_module.DeleteVariable = lambda key: nvs_store.pop(key, None) is not None
sys.modules["_MatrixOS_NVS"] = nvs_module

sys_calls = []
sys_module = types.ModuleType("_MatrixOS_SYS")
sys_module.Reboot = lambda: sys_calls.append("reboot")
sys_module.Bootloader = lambda: sys_calls.append("bootloader")
sys_module.DelayMs = lambda ms: sys_calls.append(("delay", ms))
sys_module.TaskYield = lambda: sys_calls.append("yield")
sys_module.Millis = lambda: 123
sys_module.Micros = lambda: 456
sys_module.OpenSetting = lambda: sys_calls.append("settings")
sys_module.ExecuteAPP = lambda author, app_name, args: sys_calls.append(("app", author, app_name, args))
sys_module.ExecuteAPPByID = lambda app_id, args: sys_calls.append(("app_id", app_id, args))
sys_module.GetVersion = lambda: (4, 0, 1, 99)
sys.modules["_MatrixOS_SYS"] = sys_module

usb_module = types.ModuleType("_MatrixOS_USB")
usb_module.Connected = lambda: True
sys.modules["_MatrixOS_USB"] = usb_module

import MatrixOS_HID
import MatrixOS_HID_Keyboard
import MatrixOS_HID_Gamepad
import MatrixOS_HID_RawHID
import MatrixOS_HID_Mouse
import MatrixOS_HID_Consumer
import MatrixOS_HID_System
import MatrixOS_HID_Touch
import MatrixOS_MIDI
import MatrixOS_MidiPacket
import MatrixOS_NVS
import MatrixOS_SYS
import MatrixOS_USB


def test_midi_packet_factories_and_aliases():
    packet = MatrixOS_MidiPacket.note_on(2, 60, 100)

    assert MatrixOS_MidiPacket.status(packet) == 0x90
    assert MatrixOS_MidiPacket.channel(packet) == 2
    assert MatrixOS_MidiPacket.note(packet) == 60
    assert MatrixOS_MidiPacket.velocity(packet) == 100
    assert MatrixOS_MIDI.velocity(packet) == 100

    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.cc(1, 74, 64)) == 0xB0
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.control_change(1, 74, 64)) == 0xB0
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.note_off(2, 60)) == 0x80
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.program_change(1, 8)) == 0xC0
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.channel_pressure(1, 7)) == 0xD0
    assert MatrixOS_MidiPacket.value(MatrixOS_MidiPacket.pitch_bend(3, 8192)) == 8192
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.clock()) == 0xF8
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.continue_()) == 0xFB
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.stop()) == 0xFC
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.active_sense()) == 0xFE
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.reset()) == 0xFF
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.song_position(128)) == 0xF2
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.song_select(3)) == 0xF3
    assert MatrixOS_MidiPacket.status(MatrixOS_MidiPacket.tune_request()) == 0xF6

    editable = MatrixOS_MidiPacket.MidiPacket()
    assert editable.set_status(0x90)
    assert editable.set_channel(4)
    assert editable.set_note(61)
    assert editable.set_controller(1)
    assert editable.set_velocity(111)
    assert editable.set_value(22)
    assert editable.status() == 0x90
    assert editable.channel() == 4
    assert editable.note() == 61
    assert editable.controller() == 1
    assert editable.velocity() == 111
    assert editable.value() == 22
    assert MatrixOS_MidiPacket.set_status(editable, 0x91)
    assert MatrixOS_MidiPacket.set_channel(editable, 5)
    assert MatrixOS_MidiPacket.set_note(editable, 62)
    assert MatrixOS_MidiPacket.set_controller(editable, 2)
    assert MatrixOS_MidiPacket.set_velocity(editable, 112)
    assert MatrixOS_MidiPacket.set_value(editable, 23)
    assert MatrixOS_MidiPacket.port(editable) is not None
    assert MatrixOS_MidiPacket.status(editable) == 0x91
    assert MatrixOS_MidiPacket.channel(editable) == 5
    assert MatrixOS_MidiPacket.note(editable) == 62
    assert MatrixOS_MidiPacket.controller(editable) == 2
    assert MatrixOS_MidiPacket.velocity(editable) == 112
    assert MatrixOS_MidiPacket.value(editable) == 23
    assert MatrixOS_MidiPacket.length(editable) == 3
    assert not MatrixOS_MidiPacket.is_sysex(editable)
    assert not MatrixOS_MidiPacket.is_sysex_start(editable)
    assert editable.length() == 3
    assert not editable.is_sysex()
    assert not editable.is_sysex_start()

    MatrixOS_MIDI.send(packet, 5)
    assert MatrixOS_MIDI.get() is None
    MatrixOS_MIDI.send_sysex(0x100, b"\x01\x02\x03")
    assert midi_calls[-2][0] == "send"
    assert midi_calls[-2][1] is packet.raw()
    assert midi_calls[-1] == ("sysex", 0x100, 3, b"\x01\x02\x03", False)


def test_hid_lowercase_aliases():
    hid_calls.clear()

    assert MatrixOS_HID.ready()
    assert MatrixOS_HID_Keyboard.tap(4)
    assert MatrixOS_HID_Keyboard.press(4)
    assert MatrixOS_HID_Keyboard.release(4)
    MatrixOS_HID_Keyboard.release_all()
    MatrixOS_HID_Gamepad.press(1)
    MatrixOS_HID_Gamepad.release(1)
    MatrixOS_HID_Gamepad.button(1, True)
    MatrixOS_HID_Gamepad.x_axis(12)
    MatrixOS_HID_Gamepad.y_axis(13)
    MatrixOS_HID_Gamepad.z_axis(14)
    MatrixOS_HID_Gamepad.rx_axis(15)
    MatrixOS_HID_Gamepad.ry_axis(16)
    MatrixOS_HID_Gamepad.rz_axis(17)
    MatrixOS_HID_Gamepad.buttons(0x03)
    MatrixOS_HID_Gamepad.dpad(1)
    MatrixOS_HID_Gamepad.release_all()
    MatrixOS_HID_Mouse.click(1)
    MatrixOS_HID_Mouse.press(1)
    MatrixOS_HID_Mouse.release(1)
    MatrixOS_HID_Mouse.move_to(1, 2)
    MatrixOS_HID_Mouse.move(1, 2)
    MatrixOS_HID_Mouse.release_all()
    MatrixOS_HID_Touch.click(1)
    MatrixOS_HID_Touch.press(1)
    MatrixOS_HID_Touch.release(1)
    MatrixOS_HID_Touch.move(1, 2)
    MatrixOS_HID_Touch.move_to(3, 4)
    MatrixOS_HID_Touch.release_all()
    MatrixOS_HID_Consumer.write(5)
    MatrixOS_HID_Consumer.press(5)
    MatrixOS_HID_Consumer.release(5)
    MatrixOS_HID_Consumer.release_all()
    MatrixOS_HID_System.write(5)
    MatrixOS_HID_System.press(5)
    MatrixOS_HID_System.release()
    MatrixOS_HID_System.release_all()
    assert MatrixOS_HID_RawHID.get() == b"reply"
    MatrixOS_HID_RawHID.send(b"abc")

    assert ("_MatrixOS_HID_Gamepad", "XAxis", (12,)) in hid_calls
    assert ("_MatrixOS_HID_Gamepad", "Buttons", (0x03,)) in hid_calls
    assert ("_MatrixOS_HID_Gamepad", "DPad", (1,)) in hid_calls
    assert ("_MatrixOS_HID_Mouse", "Move", (1, 2, 0)) in hid_calls
    assert ("_MatrixOS_HID_Touch", "MoveTo", (3, 4, 0)) in hid_calls
    assert ("RawHID", "Send", (b"abc", 3)) in hid_calls


def test_nvs_typed_helpers():
    assert MatrixOS_NVS.get_u32(10, 99) == 99

    assert MatrixOS_NVS.set_u8(1, 0xFE)
    assert MatrixOS_NVS.get_u8(1) == 0xFE

    assert MatrixOS_NVS.set_u16(2, 0x1234)
    assert MatrixOS_NVS.get_u16(2) == 0x1234

    assert MatrixOS_NVS.set_u32(3, 0x12345678)
    assert MatrixOS_NVS.get_u32(3) == 0x12345678

    assert MatrixOS_NVS.set_str(4, "Matrix")
    assert MatrixOS_NVS.get_str(4) == "Matrix"
    assert MatrixOS_NVS.get_size(4) == 6
    assert MatrixOS_NVS.delete(4)
    assert MatrixOS_NVS.set(5, b"abc")
    assert MatrixOS_NVS.get(5) == b"abc"
    assert MatrixOS_NVS.delete(5)


def test_sys_and_usb_facades():
    assert MatrixOS_USB.connected()
    assert MatrixOS_SYS.millis() == 123
    assert MatrixOS_SYS.micros() == 456
    MatrixOS_SYS.sleep_ms(5)
    MatrixOS_SYS.delay_ms(6)
    MatrixOS_SYS.task_yield()
    MatrixOS_SYS.yield_()
    MatrixOS_SYS.open_settings()
    assert MatrixOS_SYS.get_version() == (4, 0, 1, 99)
    MatrixOS_SYS.reboot()
    MatrixOS_SYS.bootloader()
    assert sys_calls.count("yield") == 2

    version = MatrixOS_SYS.version()
    assert isinstance(version, MatrixOS_SYS.SystemVersion)
    assert version.major() == 4
    assert version.minor() == 0
    assert version.patch() == 1
    assert version.build() == 99
    assert version.tuple() == (4, 0, 1, 99)

    MatrixOS_SYS.launch_app("203", "Note")
    MatrixOS_SYS.launch_app_by_id(7)
    assert sys_calls[-2] == ("app", "203", "Note", [])
    assert sys_calls[-1] == ("app_id", 7, [])


if __name__ == "__main__":
    test_midi_packet_factories_and_aliases()
    test_hid_lowercase_aliases()
    test_nvs_typed_helpers()
    test_sys_and_usb_facades()
