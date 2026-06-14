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


def reboot() -> None:
    _MatrixOS_SYS.Reboot()


def bootloader() -> None:
    _MatrixOS_SYS.Bootloader()


def exit_app() -> None:
    _MatrixOS_SYS.ExitAPP()


def sleep_ms(ms: int) -> None:
    _MatrixOS_SYS.DelayMs(ms)


def delay_ms(ms: int) -> None:
    _MatrixOS_SYS.DelayMs(ms)


def task_yield() -> None:
    _MatrixOS_SYS.TaskYield()


def yield_() -> None:
    _MatrixOS_SYS.TaskYield()


def millis() -> int:
    return _MatrixOS_SYS.Millis()


def micros() -> int:
    return _MatrixOS_SYS.Micros()


def open_settings() -> None:
    _MatrixOS_SYS.OpenSetting()


def launch_app(author: str, app_name: str, args: list = None) -> None:
    if args is None:
        args = []
    _MatrixOS_SYS.ExecuteAPP(author, app_name, args)


def launch_app_by_id(app_id: int, args: list = None) -> None:
    if args is None:
        args = []
    _MatrixOS_SYS.ExecuteAPPByID(app_id, args)


def get_version() -> tuple:
    return _MatrixOS_SYS.GetVersion()


def version():
    return SystemVersion(get_version())
