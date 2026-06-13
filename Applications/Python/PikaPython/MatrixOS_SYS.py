import _MatrixOS_SYS

class SystemVersion:
    def __init__(self, value):
        self.value = value

    def major(self) -> int:
        return self.value[0]

    def minor(self) -> int:
        return self.value[1]

    def patch(self) -> int:
        return self.value[2]

    def build(self) -> int:
        if len(self.value) < 4:
            return 0
        return self.value[3]

    def tuple(self):
        return self.value

def Reboot() -> None:
    _MatrixOS_SYS.Reboot()

def reboot() -> None:
    Reboot()

def Bootloader() -> None:
    _MatrixOS_SYS.Bootloader()

def bootloader() -> None:
    Bootloader()

def DelayMs(ms: int) -> None:
    _MatrixOS_SYS.DelayMs(ms)

def sleep_ms(ms: int) -> None:
    DelayMs(ms)

def delay_ms(ms: int) -> None:
    DelayMs(ms)

def Millis() -> int:
    return _MatrixOS_SYS.Millis()

def millis() -> int:
    return Millis()

def Micros() -> int:
    return _MatrixOS_SYS.Micros()

def micros() -> int:
    return Micros()

def OpenSetting() -> None:
    _MatrixOS_SYS.OpenSetting()

def open_settings() -> None:
    OpenSetting()

def ExecuteAPP(author: str, app_name: str, args: list = None) -> None:
    if args is None:
        args = []
    _MatrixOS_SYS.ExecuteAPP(author, app_name, args)

def launch_app(author: str, app_name: str, args: list = None) -> None:
    ExecuteAPP(author, app_name, args)

def ExecuteAPPByID(app_id: int, args: list = None) -> None:
    if args is None:
        args = []
    _MatrixOS_SYS.ExecuteAPPByID(app_id, args)

def launch_app_by_id(app_id: int, args: list = None) -> None:
    ExecuteAPPByID(app_id, args)
    
def GetVersion() -> tuple:
    return _MatrixOS_SYS.GetVersion()

def get_version() -> tuple:
    return GetVersion()

def version():
    return SystemVersion(GetVersion())
