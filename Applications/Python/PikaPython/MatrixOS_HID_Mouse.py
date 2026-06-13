import _MatrixOS_HID_Mouse

def Click(keycode: int) -> None:
    _MatrixOS_HID_Mouse.Click(keycode)

def Press(keycode: int) -> None:
    _MatrixOS_HID_Mouse.Press(keycode)

def Release(keycode: int) -> None:
    _MatrixOS_HID_Mouse.Release(keycode)

def ReleaseAll() -> None:
    _MatrixOS_HID_Mouse.ReleaseAll()

def MoveTo(x: int, y: int, wheel: int = 0) -> None:
    _MatrixOS_HID_Mouse.MoveTo(x, y, wheel)

def Move(x: int, y: int, wheel: int = 0) -> None:
    _MatrixOS_HID_Mouse.Move(x, y, wheel)

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
