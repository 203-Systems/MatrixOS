import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))


class NativeComponent:
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


class NativeButton(NativeComponent):
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


class NativeSelector(NativeComponent):
    def SetValueFunc(self, get_value_func):
        self.calls.append(("value_func", get_value_func))
        return True

    def SetColorFunc(self, color_func):
        self.calls.append(("color_func", color_func))
        return True

    def SetIndividualColorFunc(self, individual_color_func):
        self.calls.append(("individual_color_func", individual_color_func))
        return True

    def SetNameFunc(self, name_func):
        self.calls.append(("name_func", name_func))
        return True

    def SetLitMode(self, lit_mode):
        self.calls.append(("lit_mode", lit_mode))
        return True

    def SetDimension(self, dimension):
        self.calls.append(("dimension", dimension))
        return True

    def SetName(self, name):
        self.calls.append(("name", name))
        return True

    def SetCount(self, count):
        self.calls.append(("count", count))
        return True

    def SetDirection(self, direction):
        self.calls.append(("direction", direction))
        return True

    def SetColor(self, color):
        self.calls.append(("color", color))
        return True

    def OnChange(self, change_callback):
        self.calls.append(("change", change_callback))
        return True


class NativeNumber(NativeComponent):
    def SetName(self, name):
        self.calls.append(("name", name))
        return True

    def SetColor(self, color):
        self.calls.append(("color", color))
        return True

    def SetAlternativeColor(self, alternative_color):
        self.calls.append(("alternative_color", alternative_color))
        return True

    def SetDigits(self, digits):
        self.calls.append(("digits", digits))
        return True

    def SetSpacing(self, spacing):
        self.calls.append(("spacing", spacing))
        return True

    def SetValueFunc(self, get_value_func):
        self.calls.append(("value_func", get_value_func))
        return True

    def SetColorFunc(self, color_func):
        self.calls.append(("color_func", color_func))
        return True


component_module = types.ModuleType("_MatrixOS_UIComponent")
component_module.UIComponent = NativeComponent
sys.modules["_MatrixOS_UIComponent"] = component_module

button_module = types.ModuleType("_MatrixOS_UIButton")
button_module.UIButton = NativeButton
sys.modules["_MatrixOS_UIButton"] = button_module

selector_module = types.ModuleType("_MatrixOS_UISelector")
selector_module.UISelector = NativeSelector
selector_module.UISelectorDirection = object
selector_module.UISelectorLitMode = object
sys.modules["_MatrixOS_UISelector"] = selector_module

number_module = types.ModuleType("_MatrixOS_UI4pxNumber")
number_module.UI4pxNumber = NativeNumber
sys.modules["_MatrixOS_UI4pxNumber"] = number_module

ui_utility_calls = []
ui_utility_module = types.ModuleType("_MatrixOS_UIUtility")
ui_utility_module.TextScroll = lambda text, color, speed, loop: ui_utility_calls.append(("text", text, color, speed, loop))
ui_utility_module.NumberSelector8x8 = lambda value, color, name, lower, upper: ui_utility_calls.append(("number", value, color, name, lower, upper)) or value + 1
ui_utility_module.ColorPicker = lambda: "picked"
sys.modules["_MatrixOS_UIUtility"] = ui_utility_module

color_module = types.ModuleType("MatrixOS_Color")
color_module.Color = object
sys.modules["MatrixOS_Color"] = color_module

dimension_module = types.ModuleType("MatrixOS_Dimension")
dimension_module.Dimension = object
sys.modules["MatrixOS_Dimension"] = dimension_module

import MatrixOS_UI4pxNumber
import MatrixOS_UIButton
import MatrixOS_UIComponent
import MatrixOS_UISelector
import MatrixOS_UIUtility


def test_component_pythonic_aliases_call_native_methods():
    component = MatrixOS_UIComponent.UIComponent()

    assert component.close()
    assert component.set_enabled(False)
    assert component.set_enable_func(lambda: True)

    assert component.calls[0] == ("close",)
    assert component.calls[1] == ("enabled", False)
    assert component.calls[2][0] == "enable_func"


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


def test_selector_pythonic_aliases_call_native_methods():
    selector = MatrixOS_UISelector.UISelector()

    selector.set_value_func(lambda: 1)
    selector.set_color_func(lambda: "white")
    selector.set_individual_color_func(lambda index: "red")
    selector.set_name_func(lambda index: "name")
    selector.set_lit_mode(3)
    selector.set_dimension((2, 3))
    selector.set_name("Mode")
    selector.set_count(8)
    selector.set_direction(1)
    selector.set_color("blue")
    selector.on_change(lambda value: None)

    assert selector.calls[0][0] == "value_func"
    assert selector.calls[4] == ("lit_mode", 3)
    assert selector.calls[7] == ("count", 8)
    assert selector.calls[10][0] == "change"


def test_4px_number_pythonic_aliases_call_native_methods():
    number = MatrixOS_UI4pxNumber.UI4pxNumber()

    assert number.close()
    assert number.set_enabled(True)
    assert number.set_enable_func(lambda: True)
    number.set_name("Tempo")
    number.set_color("white")
    number.set_alternative_color("red")
    number.set_digits(3)
    number.set_spacing(1)
    number.set_value_func(lambda: 120)
    number.set_color_func(lambda: "green")

    assert number.calls[0] == ("close",)
    assert number.calls[3] == ("name", "Tempo")
    assert number.calls[5] == ("alternative_color", "red")
    assert number.calls[8][0] == "value_func"


def test_ui_utility_pythonic_aliases_call_native_methods():
    MatrixOS_UIUtility.text_scroll("Hi", "white")
    value = MatrixOS_UIUtility.number_selector_8x8(4, "red", "Value", 0, 10)
    color = MatrixOS_UIUtility.color_picker()

    assert ui_utility_calls[0] == ("text", "Hi", "white", 10, False)
    assert ui_utility_calls[1] == ("number", 4, "red", "Value", 0, 10)
    assert value == 5
    assert color == "picked"


if __name__ == "__main__":
    test_component_pythonic_aliases_call_native_methods()
    test_button_pythonic_aliases_call_native_methods()
    test_selector_pythonic_aliases_call_native_methods()
    test_4px_number_pythonic_aliases_call_native_methods()
    test_ui_utility_pythonic_aliases_call_native_methods()
