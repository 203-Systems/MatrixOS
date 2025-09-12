import _MatrixOS_HID_Mouse

def Click(keycode: int) -> None:
    _MatrixOS_HID_Mouse.Click(keycode)

def Press(keycode: int) -> None:
    _MatrixOS_HID_Mouse.Press(keycode)

def Release(keycode: int) -> None:
    _MatrixOS_HID_Mouse.Release(keycode)

def ReleaseAll() -> None:
    _MatrixOS_HID_Mouse.ReleaseAll()

def MoveTo(x: int, y: int, wheel: int) -> None:
    _MatrixOS_HID_Mouse.MoveTo(x, y, wheel)

def Move(x: int, y: int, wheel: int) -> None:
    _MatrixOS_HID_Mouse.Move(x, y, wheel)