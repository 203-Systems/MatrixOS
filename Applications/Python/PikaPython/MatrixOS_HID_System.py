import _MatrixOS_HID_System

def Write(keycode: int) -> None:
    _MatrixOS_HID_System.Write(keycode)

def Press(keycode: int) -> None:
    _MatrixOS_HID_System.Press(keycode)

def Release() -> None:
    _MatrixOS_HID_System.Release()

def ReleaseAll() -> None:
    _MatrixOS_HID_System.ReleaseAll()