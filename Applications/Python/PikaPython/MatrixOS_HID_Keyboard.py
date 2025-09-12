import _MatrixOS_HID_Keyboard

def Write(keycode: int) -> bool:
    return _MatrixOS_HID_Keyboard.Write(keycode)

def Press(keycode: int) -> bool:
    return _MatrixOS_HID_Keyboard.Press(keycode)

def Release(keycode: int) -> bool:
    return _MatrixOS_HID_Keyboard.Release(keycode)

def ReleaseAll() -> None:
    _MatrixOS_HID_Keyboard.ReleaseAll()