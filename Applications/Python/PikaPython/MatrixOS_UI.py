import _MatrixOS_UI

from MatrixOS_UIComponent import UIComponent
from MatrixOS_UIButton import UIButton
from MatrixOS_UISelector import UISelector
from MatrixOS_UI4pxNumber import UI4pxNumber

class KeyEvent:
    def __init__(self, code: int):
        self.code = code

    def ClusterId(self) -> int:
        return self.code & 0xFF

    def KeyState(self) -> int:
        return (self.code >> 8) & 0xFF

    def MemberId(self) -> int:
        return (self.code >> 16) & 0xFFFF

class UI(_MatrixOS_UI.UI):
    def PullInput(self) -> any:
        code = self.PullInputCode()
        if code < 0:
            return None
        return KeyEvent(code)
