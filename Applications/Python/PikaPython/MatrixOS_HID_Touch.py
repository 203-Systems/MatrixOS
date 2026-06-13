import _MatrixOS_HID_Touch

def Click(keycode: int) -> None:
    _MatrixOS_HID_Touch.Click(keycode)

def Press(keycode: int) -> None:
    _MatrixOS_HID_Touch.Press(keycode)

def Release(keycode: int) -> None:
    _MatrixOS_HID_Touch.Release(keycode)

def ReleaseAll() -> None:
    _MatrixOS_HID_Touch.ReleaseAll()

def MoveTo(x: int, y: int, wheel: int = 0) -> None:
    _MatrixOS_HID_Touch.MoveTo(x, y, wheel)

def Move(x: int, y: int, wheel: int = 0) -> None:
    _MatrixOS_HID_Touch.Move(x, y, wheel)

def click(keycode: int) -> None:
    Click(keycode)

def press(keycode: int) -> None:
    Press(keycode)

def release(keycode: int) -> None:
    Release(keycode)

def release_all() -> None:
    ReleaseAll()

def move_to(x: int, y: int, wheel: int = 0) -> None:
    MoveTo(x, y, wheel)

def move(x: int, y: int, wheel: int = 0) -> None:
    Move(x, y, wheel)
