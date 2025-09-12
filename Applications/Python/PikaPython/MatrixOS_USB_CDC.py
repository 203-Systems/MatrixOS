import _MatrixOS_USB_CDC

def Connected() -> bool:
    return _MatrixOS_USB_CDC.Connected()

def Available() -> int:
    return _MatrixOS_USB_CDC.Available()

def Poll() -> None:
    _MatrixOS_USB_CDC.Poll()

def Print(text: str, end: str = "\n") -> None:
    _MatrixOS_USB_CDC.Print(text, end)

def Flush() -> None:
    _MatrixOS_USB_CDC.Flush()

def Read() -> int:
    return _MatrixOS_USB_CDC.Read()

def ReadBytes(max_length: int) -> bytes:
    return _MatrixOS_USB_CDC.ReadBytes(max_length)

def ReadString() -> str:
    return _MatrixOS_USB_CDC.ReadString()