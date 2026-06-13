import _MatrixOS_HID_Keyboard

def Tap(keycode: int, length_ms: int = 100) -> bool:
    return _MatrixOS_HID_Keyboard.Tap(keycode, length_ms)

def Press(keycode: int) -> bool:
    return _MatrixOS_HID_Keyboard.Press(keycode)

def Release(keycode: int) -> bool:
    return _MatrixOS_HID_Keyboard.Release(keycode)

def ReleaseAll() -> None:
    _MatrixOS_HID_Keyboard.ReleaseAll()

def tap(keycode: int, length_ms: int = 100) -> bool:
    return Tap(keycode, length_ms)

def press(keycode: int) -> bool:
    return Press(keycode)

def release(keycode: int) -> bool:
    return Release(keycode)

def release_all() -> None:
    ReleaseAll()
