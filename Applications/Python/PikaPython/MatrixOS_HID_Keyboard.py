import _MatrixOS_HID_Keyboard


def tap(keycode: int, length_ms: int = 100) -> bool:
    return _MatrixOS_HID_Keyboard.Tap(keycode, length_ms)


def press(keycode: int) -> bool:
    return _MatrixOS_HID_Keyboard.Press(keycode)


def release(keycode: int) -> bool:
    return _MatrixOS_HID_Keyboard.Release(keycode)


def release_all() -> None:
    _MatrixOS_HID_Keyboard.ReleaseAll()
