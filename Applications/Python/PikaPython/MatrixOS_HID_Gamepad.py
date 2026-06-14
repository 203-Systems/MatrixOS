import _MatrixOS_HID_Gamepad


def tap(button_id: int, length_ms: int = 100) -> None:
    _MatrixOS_HID_Gamepad.Tap(button_id, length_ms)


def press(button_id: int) -> None:
    _MatrixOS_HID_Gamepad.Press(button_id)


def release(button_id: int) -> None:
    _MatrixOS_HID_Gamepad.Release(button_id)


def release_all() -> None:
    _MatrixOS_HID_Gamepad.ReleaseAll()


def button(button_id: int, state: bool) -> None:
    _MatrixOS_HID_Gamepad.Button(button_id, state)


def buttons(button_mask: int) -> None:
    _MatrixOS_HID_Gamepad.Buttons(button_mask)


def x_axis(value: int) -> None:
    _MatrixOS_HID_Gamepad.XAxis(value)


def y_axis(value: int) -> None:
    _MatrixOS_HID_Gamepad.YAxis(value)


def z_axis(value: int) -> None:
    _MatrixOS_HID_Gamepad.ZAxis(value)


def rx_axis(value: int) -> None:
    _MatrixOS_HID_Gamepad.RXAxis(value)


def ry_axis(value: int) -> None:
    _MatrixOS_HID_Gamepad.RYAxis(value)


def rz_axis(value: int) -> None:
    _MatrixOS_HID_Gamepad.RZAxis(value)


def dpad(direction: int) -> None:
    _MatrixOS_HID_Gamepad.DPad(direction)
