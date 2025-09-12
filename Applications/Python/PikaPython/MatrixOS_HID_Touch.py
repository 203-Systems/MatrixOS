import _MatrixOS_HID_Touch

def Click(keycode: int) -> None:
    _MatrixOS_HID_Touch.Click(keycode)

def Press(keycode: int) -> None:
    _MatrixOS_HID_Touch.Press(keycode)

def Release(keycode: int) -> None:
    _MatrixOS_HID_Touch.Release(keycode)

def ReleaseAll() -> None:
    _MatrixOS_HID_Touch.ReleaseAll()

def MoveTo(x: int, y: int, wheel: int) -> None:
    _MatrixOS_HID_Touch.MoveTo(x, y, wheel)

def Move(x: int, y: int, wheel: int) -> None:
    _MatrixOS_HID_Touch.Move(x, y, wheel)