import _MatrixOS_HID_Mouse


def click(keycode: int) -> None:
    _MatrixOS_HID_Mouse.Click(keycode)


def press(keycode: int) -> None:
    _MatrixOS_HID_Mouse.Press(keycode)


def release(keycode: int) -> None:
    _MatrixOS_HID_Mouse.Release(keycode)


def release_all() -> None:
    _MatrixOS_HID_Mouse.ReleaseAll()


def move_to(x: int, y: int, wheel: int = 0) -> None:
    _MatrixOS_HID_Mouse.MoveTo(x, y, wheel)


def move(x: int, y: int, wheel: int = 0) -> None:
    _MatrixOS_HID_Mouse.Move(x, y, wheel)
