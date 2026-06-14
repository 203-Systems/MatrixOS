import _MatrixOS_MIDI
from MatrixOS_MidiPacket import note_on, note_off, aftertouch, control_change, cc, program_change
from MatrixOS_MidiPacket import channel_pressure, pitch_bend, clock, start, continue_, stop
from MatrixOS_MidiPacket import active_sense, reset, song_position, song_select, tune_request
from MatrixOS_MidiPacket import port, status, channel, note, controller, velocity, value
from MatrixOS_MidiPacket import set_status, set_channel, set_note, set_controller, set_velocity, set_value
from MatrixOS_MidiPacket import length, is_sysex, is_sysex_start


def get(timeout_ms: int = 0):
    return _MatrixOS_MIDI.Get(timeout_ms)


def send(packet, timeout_ms: int = 0) -> bool:
    if hasattr(packet, "native"):
        packet = packet.native
    return _MatrixOS_MIDI.Send(packet, timeout_ms)


def send_sysex(port: int, data: bytes, include_meta: bool = False) -> bool:
    return _MatrixOS_MIDI.SendSysEx(port, len(data), data, include_meta)
