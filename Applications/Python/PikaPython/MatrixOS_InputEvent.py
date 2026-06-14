import _MatrixOS_InputEvent
from MatrixOS_InputId import InputIdView
from MatrixOS_KeypadInfo import KeypadInfoView


class InputEvent:
    def __init__(self):
        self.native = _MatrixOS_InputEvent.InputEvent()

    def id(self):
        return InputIdView(self.native.Id())

    def input_class(self) -> int:
        return self.native.InputClass()

    def cluster_id(self) -> int:
        return self.native.ClusterId()

    def member_id(self) -> int:
        return self.native.MemberId()

    def cluster_name(self) -> str:
        return self.id().cluster_name()

    def key_state(self) -> int:
        return self.native.KeyState()

    def key_pressure(self) -> float:
        return self.native.KeyForce()

    def key_velocity(self) -> float:
        return self.native.KeyValue(1)

    def key_value(self, index: int) -> float:
        return self.native.KeyValue(index)

    def key_hold(self) -> bool:
        return self.native.KeyHold()

    def key_active(self) -> bool:
        return self.native.KeyActive()

    def keypad(self):
        keypad = self.native.Keypad()
        if keypad is None:
            return None
        return KeypadInfoView(keypad)

    def info(self):
        return self.keypad()

    def raw(self):
        return self.native

    def is_pressed(self) -> bool:
        return self.native.KeyState() == 2

    def is_hold(self) -> bool:
        return self.native.KeyState() == 3

    def is_aftertouch(self) -> bool:
        return self.native.KeyState() == 4

    def is_released(self) -> bool:
        return self.native.KeyState() == 5

    def is_function_key(self) -> bool:
        return self.id().is_function_key()


class InputEventView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_InputEvent.InputEvent()
        self.native = native

    def id(self):
        return InputIdView(self.native.Id())

    def input_class(self) -> int:
        return self.native.InputClass()

    def cluster_id(self) -> int:
        return self.native.ClusterId()

    def member_id(self) -> int:
        return self.native.MemberId()

    def cluster_name(self) -> str:
        return self.id().cluster_name()

    def key_state(self) -> int:
        return self.native.KeyState()

    def key_pressure(self) -> float:
        return self.native.KeyForce()

    def key_velocity(self) -> float:
        return self.native.KeyValue(1)

    def key_value(self, index: int) -> float:
        return self.native.KeyValue(index)

    def key_hold(self) -> bool:
        return self.native.KeyHold()

    def key_active(self) -> bool:
        return self.native.KeyActive()

    def keypad(self):
        keypad = self.native.Keypad()
        if keypad is None:
            return None
        return KeypadInfoView(keypad)

    def info(self):
        return self.keypad()

    def raw(self):
        return self.native

    def is_pressed(self) -> bool:
        return self.native.KeyState() == 2

    def is_hold(self) -> bool:
        return self.native.KeyState() == 3

    def is_aftertouch(self) -> bool:
        return self.native.KeyState() == 4

    def is_released(self) -> bool:
        return self.native.KeyState() == 5

    def is_function_key(self) -> bool:
        return self.id().is_function_key()

    def __bool__(self) -> bool:
        return bool(self.native)
