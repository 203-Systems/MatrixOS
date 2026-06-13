import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))


class NativeButton:
    def __init__(self):
        self.calls = []

    def Close(self):
        self.calls.append(("close",))
        return True

    def SetEnabled(self, enabled):
        self.calls.append(("enabled", enabled))
        return True

    def SetEnableFunc(self, enable_func):
        self.calls.append(("enable_func", enable_func))
        return True

    def SetName(self, name):
        self.calls.append(("name", name))
        return True

    def SetColor(self, color):
        self.calls.append(("color", color))
        return True

    def SetColorFunc(self, color_func):
        self.calls.append(("color_func", color_func))
        return True

    def SetSize(self, dimension):
        self.calls.append(("size", dimension))
        return True

    def OnPress(self, press_callback):
        self.calls.append(("press", press_callback))
        return True

    def OnHold(self, hold_callback):
        self.calls.append(("hold", hold_callback))
        return True


button_module = types.ModuleType("_MatrixOS_UIButton")
button_module.UIButton = NativeButton
sys.modules["_MatrixOS_UIButton"] = button_module

import MatrixOS_UIButton


def test_button_pythonic_aliases_call_native_methods():
    button = MatrixOS_UIButton.UIButton()

    button.set_enabled(True)
    button.set_enable_func(lambda: True)
    button.set_name("Play")
    button.set_color("white")
    button.set_color_func(lambda: "white")
    button.set_size((1, 2))
    button.on_press(lambda: None)
    button.on_hold(lambda: None)

    assert button.calls[0] == ("enabled", True)
    assert button.calls[2] == ("name", "Play")
    assert button.calls[3] == ("color", "white")
    assert button.calls[5] == ("size", (1, 2))
    assert button.calls[6][0] == "press"
    assert button.calls[7][0] == "hold"


if __name__ == "__main__":
    test_button_pythonic_aliases_call_native_methods()
