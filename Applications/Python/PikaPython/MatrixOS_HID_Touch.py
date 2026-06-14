import _MatrixOS_HID_Touch


def click(keycode: int) -> None:
    _MatrixOS_HID_Touch.Click(keycode)


def press(keycode: int) -> None:
    _MatrixOS_HID_Touch.Press(keycode)


def release(keycode: int) -> None:
    _MatrixOS_HID_Touch.Release(keycode)


def release_all() -> None:
    _MatrixOS_HID_Touch.ReleaseAll()


def move_to(x: int, y: int, wheel: int = 0) -> None:
    _MatrixOS_HID_Touch.MoveTo(x, y, wheel)


def move(x: int, y: int, wheel: int = 0) -> None:
    _MatrixOS_HID_Touch.Move(x, y, wheel)
