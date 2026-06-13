import _MatrixOS_InputEvent
from MatrixOS_InputId import InputIdView
from MatrixOS_KeypadInfo import KeypadInfoView

class InputEvent(_MatrixOS_InputEvent.InputEvent):
    def id(self):
        return self.Id()

    def input_class(self) -> int:
        return self.InputClass()

    def cluster_id(self) -> int:
        return self.ClusterId()

    def member_id(self) -> int:
        return self.MemberId()

    def input_id(self) -> int:
        return self.MemberId()

    def key_state(self) -> int:
        return self.KeyState()

    def key_force(self) -> float:
        return self.KeyForce()

    def key_value(self, index: int) -> float:
        return self.KeyValue(index)

    def key_hold(self) -> bool:
        return self.KeyHold()

    def key_active(self) -> bool:
        return self.KeyActive()

    def keypad(self):
        return self.Keypad()

    def info(self):
        return self.Keypad()

    def raw(self):
        return self

    def is_pressed(self) -> bool:
        return self.KeyState() == 2

    def is_hold(self) -> bool:
        return self.KeyState() == 3

    def is_aftertouch(self) -> bool:
        return self.KeyState() == 4

    def is_released(self) -> bool:
        return self.KeyState() == 5

class InputEventView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_InputEvent.InputEvent()
        self.native = native

    def Id(self):
        return InputIdView(self.native.Id())

    def id(self):
        return self.Id()

    def InputClass(self) -> int:
        return self.native.InputClass()

    def input_class(self) -> int:
        return self.InputClass()

    def ClusterId(self) -> int:
        return self.native.ClusterId()

    def cluster_id(self) -> int:
        return self.ClusterId()

    def MemberId(self) -> int:
        return self.native.MemberId()

    def member_id(self) -> int:
        return self.MemberId()

    def input_id(self) -> int:
        return self.MemberId()

    def KeyState(self) -> int:
        return self.native.KeyState()

    def key_state(self) -> int:
        return self.KeyState()

    def KeyForce(self) -> float:
        return self.native.KeyForce()

    def key_force(self) -> float:
        return self.KeyForce()

    def KeyValue(self, index: int) -> float:
        return self.native.KeyValue(index)

    def key_value(self, index: int) -> float:
        return self.KeyValue(index)

    def KeyHold(self) -> bool:
        return self.native.KeyHold()

    def key_hold(self) -> bool:
        return self.KeyHold()

    def KeyActive(self) -> bool:
        return self.native.KeyActive()

    def key_active(self) -> bool:
        return self.KeyActive()

    def Keypad(self):
        keypad = self.native.Keypad()
        if keypad is None:
            return None
        return KeypadInfoView(keypad)

    def keypad(self):
        return self.Keypad()

    def info(self):
        return self.Keypad()

    def raw(self):
        return self.native

    def is_pressed(self) -> bool:
        return self.KeyState() == 2

    def is_hold(self) -> bool:
        return self.KeyState() == 3

    def is_aftertouch(self) -> bool:
        return self.KeyState() == 4

    def is_released(self) -> bool:
        return self.KeyState() == 5

    def __bool__(self) -> bool:
        return bool(self.native)
