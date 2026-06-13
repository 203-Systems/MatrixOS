import _MatrixOS_KeypadInfo

class KeypadInfo(_MatrixOS_KeypadInfo.KeypadInfo):
    def state(self) -> int:
        return self.State()

    def force(self) -> float:
        return self.Force()

    def value(self, index: int) -> float:
        return self.Value(index)

    def velocity(self) -> float:
        return self.Value(1)

    def last_event_time(self) -> int:
        return self.LastEventTime()

    def hold(self) -> bool:
        return self.Hold()

    def active(self) -> bool:
        return self.Active()

    def hold_time(self) -> int:
        return self.HoldTime()

    def raw(self):
        return self

class KeypadInfoView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_KeypadInfo.KeypadInfo()
        self.native = native

    def State(self) -> int:
        return self.native.State()

    def state(self) -> int:
        return self.State()

    def Force(self) -> float:
        return self.native.Force()

    def force(self) -> float:
        return self.Force()

    def Value(self, index: int) -> float:
        return self.native.Value(index)

    def value(self, index: int) -> float:
        return self.Value(index)

    def velocity(self) -> float:
        return self.Value(1)

    def LastEventTime(self) -> int:
        return self.native.LastEventTime()

    def last_event_time(self) -> int:
        return self.LastEventTime()

    def Hold(self) -> bool:
        return self.native.Hold()

    def hold(self) -> bool:
        return self.Hold()

    def Active(self) -> bool:
        return self.native.Active()

    def active(self) -> bool:
        return self.Active()

    def HoldTime(self) -> int:
        return self.native.HoldTime()

    def hold_time(self) -> int:
        return self.HoldTime()

    def raw(self):
        return self.native

    def __bool__(self) -> bool:
        return bool(self.native)
