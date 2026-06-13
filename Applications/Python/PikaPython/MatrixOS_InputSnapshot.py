import _MatrixOS_InputSnapshot
from MatrixOS_InputId import InputIdView
from MatrixOS_KeypadInfo import KeypadInfoView

class InputSnapshot(_MatrixOS_InputSnapshot.InputSnapshot):
    def id(self):
        return self.Id()

    def input_class(self) -> int:
        return self.InputClass()

    def Keypad(self):
        return super().Keypad()

    def keypad(self):
        return self.Keypad()

    def info(self):
        return self.Keypad()

    def raw(self):
        return self

class InputSnapshotView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_InputSnapshot.InputSnapshot()
        self.native = native

    def Id(self):
        return InputIdView(self.native.Id())

    def id(self):
        return self.Id()

    def InputClass(self) -> int:
        return self.native.InputClass()

    def input_class(self) -> int:
        return self.InputClass()

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

    def __bool__(self) -> bool:
        return bool(self.native)
