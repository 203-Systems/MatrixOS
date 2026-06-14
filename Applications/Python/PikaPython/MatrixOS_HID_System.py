import _MatrixOS_HID_System


def write(keycode: int) -> None:
    _MatrixOS_HID_System.Write(keycode)


def press(keycode: int) -> None:
    _MatrixOS_HID_System.Press(keycode)


def release() -> None:
    _MatrixOS_HID_System.Release()


def release_all() -> None:
    _MatrixOS_HID_System.ReleaseAll()
