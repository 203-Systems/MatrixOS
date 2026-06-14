import _MatrixOS_InputSnapshot
from MatrixOS_InputId import InputIdView
from MatrixOS_KeypadInfo import KeypadInfoView


class InputSnapshot:
    def __init__(self):
        self.native = _MatrixOS_InputSnapshot.InputSnapshot()

    def id(self):
        return InputIdView(self.native.Id())

    def input_class(self) -> int:
        return self.native.InputClass()

    def keypad(self):
        keypad = self.native.Keypad()
        if keypad is None:
            return None
        return KeypadInfoView(keypad)

    def info(self):
        return self.keypad()

    def raw(self):
        return self.native


class InputSnapshotView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_InputSnapshot.InputSnapshot()
        self.native = native

    def id(self):
        return InputIdView(self.native.Id())

    def input_class(self) -> int:
        return self.native.InputClass()

    def keypad(self):
        keypad = self.native.Keypad()
        if keypad is None:
            return None
        return KeypadInfoView(keypad)

    def info(self):
        return self.keypad()

    def raw(self):
        return self.native

    def __bool__(self) -> bool:
        return bool(self.native)
