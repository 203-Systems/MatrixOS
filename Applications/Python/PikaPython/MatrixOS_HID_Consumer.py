import _MatrixOS_HID_Consumer


def write(keycode: int) -> None:
    _MatrixOS_HID_Consumer.Write(keycode)


def press(keycode: int) -> None:
    _MatrixOS_HID_Consumer.Press(keycode)


def release(keycode: int) -> None:
    _MatrixOS_HID_Consumer.Release(keycode)


def release_all() -> None:
    _MatrixOS_HID_Consumer.ReleaseAll()
