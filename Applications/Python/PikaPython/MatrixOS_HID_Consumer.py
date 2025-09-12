import _MatrixOS_HID_Consumer

def Write(keycode: int) -> None:
    _MatrixOS_HID_Consumer.Write(keycode)

def Press(keycode: int) -> None:
    _MatrixOS_HID_Consumer.Press(keycode)

def Release(keycode: int) -> None:
    _MatrixOS_HID_Consumer.Release(keycode)

def ReleaseAll() -> None:
    _MatrixOS_HID_Consumer.ReleaseAll()