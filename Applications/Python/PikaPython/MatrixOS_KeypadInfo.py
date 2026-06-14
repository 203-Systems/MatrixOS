import _MatrixOS_KeypadInfo


class KeypadInfo:
    def __init__(self):
        self.native = _MatrixOS_KeypadInfo.KeypadInfo()

    def state(self) -> int:
        return self.native.State()

    def pressure(self) -> float:
        return self.native.Force()

    def force(self) -> float:
        return self.pressure()

    def value(self, index: int) -> float:
        return self.native.Value(index)

    def velocity(self) -> float:
        return self.native.Value(1)

    def last_event_time(self) -> int:
        return self.native.LastEventTime()

    def hold(self) -> bool:
        return self.native.Hold()

    def active(self) -> bool:
        return self.native.Active()

    def hold_time(self) -> int:
        return self.native.HoldTime()

    def raw(self):
        return self.native


class KeypadInfoView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_KeypadInfo.KeypadInfo()
        self.native = native

    def state(self) -> int:
        return self.native.State()

    def pressure(self) -> float:
        return self.native.Force()

    def force(self) -> float:
        return self.pressure()

    def value(self, index: int) -> float:
        return self.native.Value(index)

    def velocity(self) -> float:
        return self.native.Value(1)

    def last_event_time(self) -> int:
        return self.native.LastEventTime()

    def hold(self) -> bool:
        return self.native.Hold()

    def active(self) -> bool:
        return self.native.Active()

    def hold_time(self) -> int:
        return self.native.HoldTime()

    def raw(self):
        return self.native

    def __bool__(self) -> bool:
        return bool(self.native)
