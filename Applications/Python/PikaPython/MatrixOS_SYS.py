import _MatrixOS_SYS

def Reboot() -> None:
    _MatrixOS_SYS.Reboot()

def Bootloader() -> None:
    _MatrixOS_SYS.Bootloader()

def DelayMs(ms: int) -> None:
    _MatrixOS_SYS.DelayMs(ms)

def Millis() -> int:
    return _MatrixOS_SYS.Millis()

def Micros() -> int:
    return _MatrixOS_SYS.Micros()

def OpenSetting() -> None:
    _MatrixOS_SYS.OpenSetting()

def ExecuteAPP(author: str, app_name: str, args: list = []) -> None:
    _MatrixOS_SYS.ExecuteAPP(author, app_name, args)

def ExecuteAPPByID(app_id: int, args: list = []) -> None:
    _MatrixOS_SYS.ExecuteAPPByID(app_id, args)
    
def GetVersion() -> tuple:
    return _MatrixOS_SYS.GetVersion()