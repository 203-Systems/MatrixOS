import _MatrixOS_HID_RawHID

def Get(timeout_ms: int = 0) -> bytes:
    return _MatrixOS_HID_RawHID.Get(timeout_ms)

def Send(report: bytes) -> bool:
    return _MatrixOS_HID_RawHID.Send(report)