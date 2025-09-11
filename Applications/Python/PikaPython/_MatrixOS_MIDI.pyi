# MatrixOS Python Interface - MIDI
# MIDI communication functions

# Note: MidiPacket type needs to be defined elsewhere
class MidiPacket:
    pass

def Get(self, timeout_ms: int) -> MidiPacket: ...
def Send(self, packet: MidiPacket, timeout_ms: int) -> bool: ...
def SendSysEx(self, port: int, length: int, data: bytes, include_meta: bool) -> bool: ...