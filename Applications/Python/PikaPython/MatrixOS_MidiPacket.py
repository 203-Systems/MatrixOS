import _MatrixOS_MidiPacket
from MatrixOS_MidiPortID import MidiPortID
from MatrixOS_MidiStatus import MidiStatus


def _native(packet):
    if hasattr(packet, "native"):
        return packet.native
    return packet


def _wrap(native):
    packet = MidiPacket()
    packet.native = native
    return packet


class MidiPacket:
    def __init__(self, *args):
        self.native = _MatrixOS_MidiPacket.MidiPacket(*args)

    def note_on(self, channel: int, note: int, velocity: int = 127):
        return _wrap(self.native.NoteOn(channel, note, velocity))

    def note_off(self, channel: int, note: int, velocity: int = 0):
        return _wrap(self.native.NoteOff(channel, note, velocity))

    def aftertouch(self, channel: int, note: int, pressure: int):
        return _wrap(self.native.AfterTouch(channel, note, pressure))

    def control_change(self, channel: int, controller: int, value: int):
        return _wrap(self.native.ControlChange(channel, controller, value))

    def program_change(self, channel: int, program: int):
        return _wrap(self.native.ProgramChange(channel, program))

    def channel_pressure(self, channel: int, pressure: int):
        return _wrap(self.native.ChannelPressure(channel, pressure))

    def pitch_bend(self, channel: int, value: int):
        return _wrap(self.native.PitchBend(channel, value))

    def port(self) -> int:
        return self.native.Port()

    def status(self) -> int:
        return self.native.Status()

    def channel(self) -> int:
        return self.native.Channel()

    def note(self) -> int:
        return self.native.Note()

    def controller(self) -> int:
        return self.native.Controller()

    def velocity(self) -> int:
        return self.native.Velocity()

    def value(self) -> int:
        return self.native.Value()

    def set_status(self, status: int) -> bool:
        return self.native.SetStatus(status)

    def set_channel(self, channel: int) -> bool:
        return self.native.SetChannel(channel)

    def set_note(self, note: int) -> bool:
        return self.native.SetNote(note)

    def set_controller(self, controller: int) -> bool:
        return self.native.SetController(controller)

    def set_velocity(self, velocity: int) -> bool:
        return self.native.SetVelocity(velocity)

    def set_value(self, value: int) -> bool:
        return self.native.SetValue(value)

    def length(self) -> int:
        return self.native.Length()

    def is_sysex(self) -> bool:
        return self.native.SysEx()

    def is_sysex_start(self) -> bool:
        return self.native.SysExStart()

    def raw(self):
        return self.native


def note_on(channel: int, note: int, velocity: int = 127):
    return MidiPacket().note_on(channel, note, velocity)


def note_off(channel: int, note: int, velocity: int = 0):
    return MidiPacket().note_off(channel, note, velocity)


def aftertouch(channel: int, note: int, pressure: int):
    return MidiPacket().aftertouch(channel, note, pressure)


def control_change(channel: int, controller: int, value: int):
    return MidiPacket().control_change(channel, controller, value)


def cc(channel: int, controller: int, value: int):
    return control_change(channel, controller, value)


def program_change(channel: int, program: int):
    return MidiPacket().program_change(channel, program)


def channel_pressure(channel: int, pressure: int):
    return MidiPacket().channel_pressure(channel, pressure)


def pitch_bend(channel: int, value: int):
    return MidiPacket().pitch_bend(channel, value)


def clock():
    return _wrap(MidiPacket().native.Clock())


def start():
    return _wrap(MidiPacket().native.Start())


def continue_():
    return _wrap(MidiPacket().native.Continue())


def stop():
    return _wrap(MidiPacket().native.Stop())


def active_sense():
    return _wrap(MidiPacket().native.ActiveSense())


def reset():
    return _wrap(MidiPacket().native.Reset())


def song_position(position: int):
    return _wrap(MidiPacket().native.SongPosition(position))


def song_select(song: int):
    return _wrap(MidiPacket().native.SongSelect(song))


def tune_request():
    return _wrap(MidiPacket().native.TuneRequest())


def port(packet) -> int:
    return _native(packet).Port()


def status(packet) -> int:
    return _native(packet).Status()


def channel(packet) -> int:
    return _native(packet).Channel()


def note(packet) -> int:
    return _native(packet).Note()


def controller(packet) -> int:
    return _native(packet).Controller()


def velocity(packet) -> int:
    return _native(packet).Velocity()


def value(packet) -> int:
    return _native(packet).Value()


def set_status(packet, status_value: int) -> bool:
    return _native(packet).SetStatus(status_value)


def set_channel(packet, channel_value: int) -> bool:
    return _native(packet).SetChannel(channel_value)


def set_note(packet, note_value: int) -> bool:
    return _native(packet).SetNote(note_value)


def set_controller(packet, controller_value: int) -> bool:
    return _native(packet).SetController(controller_value)


def set_velocity(packet, velocity_value: int) -> bool:
    return _native(packet).SetVelocity(velocity_value)


def set_value(packet, value_value: int) -> bool:
    return _native(packet).SetValue(value_value)


def length(packet) -> int:
    return _native(packet).Length()


def is_sysex(packet) -> bool:
    return _native(packet).SysEx()


def is_sysex_start(packet) -> bool:
    return _native(packet).SysExStart()
