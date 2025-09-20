import _MatrixOS_HID_Gamepad

def Tap(button_id: int, length_ms: int = 100) -> None:
    _MatrixOS_HID_Gamepad.Tap(button_id, length_ms)

def Press(button_id: int) -> None:
    _MatrixOS_HID_Gamepad.Press(button_id)

def Release(button_id: int) -> None:
    _MatrixOS_HID_Gamepad.Release(button_id)

def ReleaseAll() -> None:
    _MatrixOS_HID_Gamepad.ReleaseAll()

def Button(button_id: int, state: bool) -> None:
    _MatrixOS_HID_Gamepad.Button(button_id, state)

def Buttons(button_mask: int) -> None:
    _MatrixOS_HID_Gamepad.Buttons(button_mask)

def XAxis(value: int) -> None:
    _MatrixOS_HID_Gamepad.XAxis(value)

def YAxis(value: int) -> None:
    _MatrixOS_HID_Gamepad.YAxis(value)

def ZAxis(value: int) -> None:
    _MatrixOS_HID_Gamepad.ZAxis(value)

def RXAxis(value: int) -> None:
    _MatrixOS_HID_Gamepad.RXAxis(value)

def RYAxis(value: int) -> None:
    _MatrixOS_HID_Gamepad.RYAxis(value)

def RZAxis(value: int) -> None:
    _MatrixOS_HID_Gamepad.RZAxis(value)

def DPad(direction: int) -> None:
    _MatrixOS_HID_Gamepad.DPad(direction)