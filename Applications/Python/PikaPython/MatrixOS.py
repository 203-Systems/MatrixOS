import PikaStdLib
import _MatrixOS

def Reboot() -> None:
    _MatrixOS.Reboot()

def Bootloader() -> None:
    _MatrixOS.Bootloader()

def DelayMs(ms: int) -> None:
    _MatrixOS.DelayMs(ms)

def Millis() -> int:
    return _MatrixOS.Millis()

def OpenSetting() -> None:
    _MatrixOS.OpenSetting()

def ExecuteAPP(author: str, app_name: str) -> None:
    _MatrixOS.ExecuteAPP(author, app_name)

def ExecuteAPPByID(app_id: int) -> None:
    _MatrixOS.ExecuteAPPByID(app_id)

def Test(input: int = 100) -> int:
    return _MatrixOS.Test(input)

class KeyPad:
    def Get(self, timeout_ms: int = 0) -> KeyEvent: ...
    def GetKey(self, keyXY: Point) -> KeyInfo: ...
    def GetKeyByID(self, keyID: int) -> KeyInfo: ...
    def Clear(self) -> None: ...
    def XY2ID(self, xy: Point) -> int: ...
    def ID2XY(self, keyID: int) -> Point: ...