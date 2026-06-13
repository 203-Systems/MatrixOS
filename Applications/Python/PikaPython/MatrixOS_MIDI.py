import _MatrixOS_MIDI

def Get(timeout_ms: int = 0):
    return _MatrixOS_MIDI.Get(timeout_ms)

def get(timeout_ms: int = 0):
    return Get(timeout_ms)

def Send(packet, timeout_ms: int = 0) -> bool:
    return _MatrixOS_MIDI.Send(packet, timeout_ms)

def send(packet, timeout_ms: int = 0) -> bool:
    return Send(packet, timeout_ms)

def SendSysEx(port: int, length: int, data: bytes, include_meta: bool = False) -> bool:
    return _MatrixOS_MIDI.SendSysEx(port, length, data, include_meta)

def send_sysex(port: int, data: bytes, include_meta: bool = False) -> bool:
    return SendSysEx(port, len(data), data, include_meta)
