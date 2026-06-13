import _MatrixOS_MidiPacket
from MatrixOS_MidiPortID import MidiPortID
from MatrixOS_MidiStatus import MidiStatus

class MidiPacket(_MatrixOS_MidiPacket.MidiPacket):
    def NoteOn(self, channel: int, note: int, velocity: int = 127):
        return super().NoteOn(channel, note, velocity)

    def NoteOff(self, channel: int, note: int, velocity: int = 0):
        return super().NoteOff(channel, note, velocity)

    def AfterTouch(self, channel: int, note: int, pressure: int):
        return super().AfterTouch(channel, note, pressure)

    def ControlChange(self, channel: int, controller: int, value: int):
        return super().ControlChange(channel, controller, value)

    def ProgramChange(self, channel: int, program: int):
        return super().ProgramChange(channel, program)

    def ChannelPressure(self, channel: int, pressure: int):
        return super().ChannelPressure(channel, pressure)

    def PitchBend(self, channel: int, value: int):
        return super().PitchBend(channel, value)

    def port(self) -> int:
        return self.Port()

    def status(self) -> int:
        return self.Status()

    def channel(self) -> int:
        return self.Channel()

    def note(self) -> int:
        return self.Note()

    def controller(self) -> int:
        return self.Controller()

    def velocity(self) -> int:
        return self.Velocity()

    def value(self) -> int:
        return self.Value()

    def set_status(self, status: int) -> bool:
        return self.SetStatus(status)

    def set_channel(self, channel: int) -> bool:
        return self.SetChannel(channel)

    def set_note(self, note: int) -> bool:
        return self.SetNote(note)

    def set_controller(self, controller: int) -> bool:
        return self.SetController(controller)

    def set_velocity(self, velocity: int) -> bool:
        return self.SetVelocity(velocity)

    def set_value(self, value: int) -> bool:
        return self.SetValue(value)

    def length(self) -> int:
        return self.Length()

    def is_sysex(self) -> bool:
        return self.SysEx()

    def is_sysex_start(self) -> bool:
        return self.SysExStart()

def note_on(channel: int, note: int, velocity: int = 127):
    return MidiPacket().NoteOn(channel, note, velocity)

def note_off(channel: int, note: int, velocity: int = 0):
    return MidiPacket().NoteOff(channel, note, velocity)

def aftertouch(channel: int, note: int, pressure: int):
    return MidiPacket().AfterTouch(channel, note, pressure)

def control_change(channel: int, controller: int, value: int):
    return MidiPacket().ControlChange(channel, controller, value)

def cc(channel: int, controller: int, value: int):
    return control_change(channel, controller, value)

def program_change(channel: int, program: int):
    return MidiPacket().ProgramChange(channel, program)

def channel_pressure(channel: int, pressure: int):
    return MidiPacket().ChannelPressure(channel, pressure)

def pitch_bend(channel: int, value: int):
    return MidiPacket().PitchBend(channel, value)

def clock():
    return MidiPacket().Clock()

def start():
    return MidiPacket().Start()

def continue_():
    return MidiPacket().Continue()

def stop():
    return MidiPacket().Stop()

def active_sense():
    return MidiPacket().ActiveSense()

def reset():
    return MidiPacket().Reset()

def song_position(position: int):
    return MidiPacket().SongPosition(position)

def song_select(song: int):
    return MidiPacket().SongSelect(song)

def tune_request():
    return MidiPacket().TuneRequest()

def port(packet) -> int:
    return packet.Port()

def status(packet) -> int:
    return packet.Status()

def channel(packet) -> int:
    return packet.Channel()

def note(packet) -> int:
    return packet.Note()

def controller(packet) -> int:
    return packet.Controller()

def velocity(packet) -> int:
    return packet.Velocity()

def value(packet) -> int:
    return packet.Value()

def set_status(packet, status_value: int) -> bool:
    return packet.SetStatus(status_value)

def set_channel(packet, channel_value: int) -> bool:
    return packet.SetChannel(channel_value)

def set_note(packet, note_value: int) -> bool:
    return packet.SetNote(note_value)

def set_controller(packet, controller_value: int) -> bool:
    return packet.SetController(controller_value)

def set_velocity(packet, velocity_value: int) -> bool:
    return packet.SetVelocity(velocity_value)

def set_value(packet, value_value: int) -> bool:
    return packet.SetValue(value_value)

def length(packet) -> int:
    return packet.Length()

def is_sysex(packet) -> bool:
    return packet.SysEx()

def is_sysex_start(packet) -> bool:
    return packet.SysExStart()
