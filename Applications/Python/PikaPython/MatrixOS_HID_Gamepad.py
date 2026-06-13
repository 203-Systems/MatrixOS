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

def tap(button_id: int, length_ms: int = 100) -> None:
    Tap(button_id, length_ms)

def press(button_id: int) -> None:
    Press(button_id)

def release(button_id: int) -> None:
    Release(button_id)

def release_all() -> None:
    ReleaseAll()

def button(button_id: int, state: bool) -> None:
    Button(button_id, state)

def buttons(button_mask: int) -> None:
    Buttons(button_mask)

def x_axis(value: int) -> None:
    XAxis(value)

def y_axis(value: int) -> None:
    YAxis(value)

def z_axis(value: int) -> None:
    ZAxis(value)

def rx_axis(value: int) -> None:
    RXAxis(value)

def ry_axis(value: int) -> None:
    RYAxis(value)

def rz_axis(value: int) -> None:
    RZAxis(value)

def dpad(direction: int) -> None:
    DPad(direction)
