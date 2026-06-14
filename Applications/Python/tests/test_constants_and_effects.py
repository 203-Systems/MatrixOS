import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))


class NativeColor:
    def __init__(self, value=0):
        self.value = value


color_module = types.ModuleType("_MatrixOS_Color")
color_module.Color = NativeColor
sys.modules["_MatrixOS_Color"] = color_module

class NativePoint:
    pass

class NativeDimension:
    pass

class NativeKeypadInfo:
    pass

class NativeInputEvent:
    pass

class NativeInputSnapshot:
    pass

class NativeInputCluster:
    pass

class NativeMidiPacket:
    pass

class NativeInputId:
    pass

native_class_stubs = {
    "_MatrixOS_Point": ("Point", NativePoint),
    "_MatrixOS_Dimension": ("Dimension", NativeDimension),
    "_MatrixOS_KeypadInfo": ("KeypadInfo", NativeKeypadInfo),
    "_MatrixOS_InputEvent": ("InputEvent", NativeInputEvent),
    "_MatrixOS_InputSnapshot": ("InputSnapshot", NativeInputSnapshot),
    "_MatrixOS_InputCluster": ("InputCluster", NativeInputCluster),
    "_MatrixOS_MidiPacket": ("MidiPacket", NativeMidiPacket),
    "_MatrixOS_InputId": ("InputId", NativeInputId),
}

for module_name, class_info in native_class_stubs.items():
    module = types.ModuleType(module_name)
    setattr(module, class_info[0], class_info[1])
    sys.modules[module_name] = module

effect_calls = []
effects_module = types.ModuleType("_MatrixOS_ColorEffects")
effects_module.Rainbow = lambda period, offset: effect_calls.append(("rainbow", period, offset)) or NativeColor(1)
effects_module.Breath = lambda period, offset: effect_calls.append(("breath", period, offset)) or 2
effects_module.BreathLowBound = lambda low, period, offset: effect_calls.append(("breath_low", low, period, offset)) or 3
effects_module.Strobe = lambda period, offset: effect_calls.append(("strobe", period, offset)) or 4
effects_module.Saw = lambda period, offset: effect_calls.append(("saw", period, offset)) or 5
effects_module.Triangle = lambda period, offset: effect_calls.append(("triangle", period, offset)) or 6
effects_module.ColorBreath = lambda color, period, offset: effect_calls.append(("color_breath", color, period, offset)) or color
effects_module.ColorBreathLowBound = lambda color, low, period, offset: effect_calls.append(("color_breath_low", color, low, period, offset)) or color
effects_module.ColorStrobe = lambda color, period, offset: effect_calls.append(("color_strobe", color, period, offset)) or color
effects_module.ColorSaw = lambda color, period, offset: effect_calls.append(("color_saw", color, period, offset)) or color
effects_module.ColorTriangle = lambda color, period, offset: effect_calls.append(("color_triangle", color, period, offset)) or color
sys.modules["_MatrixOS_ColorEffects"] = effects_module

utils_module = types.ModuleType("_MatrixOS_Utils")
utils_module.StringHash = lambda text: len(text)
sys.modules["_MatrixOS_Utils"] = utils_module

input_class_native_module = types.ModuleType("_MatrixOS_InputClass")
sys.modules["_MatrixOS_InputClass"] = input_class_native_module

sys_time = {"millis": 1000}
sys_module = types.ModuleType("_MatrixOS_SYS")
sys_module.Millis = lambda: sys_time["millis"]
sys_module.Micros = lambda: sys_time["millis"] * 1000
sys_module.DelayMs = lambda ms: sys_time.__setitem__("millis", sys_time["millis"] + ms)
sys_module.TaskYield = lambda: None
sys_module.Reboot = lambda: None
sys_module.Bootloader = lambda: None
sys_module.OpenSetting = lambda: None
sys_module.ExecuteAPP = lambda author, app_name, args: None
sys_module.ExecuteAPPByID = lambda app_id, args: None
sys_module.GetVersion = lambda: (4, 0, 1)
sys.modules["_MatrixOS_SYS"] = sys_module

import MatrixOS_ColorEffects
import MatrixOS_Color
import MatrixOS_ConsumerKeycode
import MatrixOS_Direction
import MatrixOS_Framework
import MatrixOS_GamepadDPadDirection
import MatrixOS_InputClass
import MatrixOS_KeyState
import MatrixOS_KeyboardKeycode
import MatrixOS_MouseKeycode
import MatrixOS_SystemKeycode
import MatrixOS_Utils


def test_color_effect_lowercase_aliases():
    color = MatrixOS_Color.Color(7)

    assert MatrixOS_ColorEffects.rainbow().raw().value == 1
    assert MatrixOS_ColorEffects.breath(10, 1) == 2
    assert MatrixOS_ColorEffects.breath_low_bound(8, 10, 1) == 3
    assert MatrixOS_ColorEffects.strobe(10, 1) == 4
    assert MatrixOS_ColorEffects.saw(10, 1) == 5
    assert MatrixOS_ColorEffects.triangle(10, 1) == 6
    assert MatrixOS_ColorEffects.color_breath(color).raw() is color.raw()
    assert MatrixOS_ColorEffects.color_breath_low_bound(color).raw() is color.raw()
    assert MatrixOS_ColorEffects.color_strobe(color).raw() is color.raw()
    assert MatrixOS_ColorEffects.color_saw(color).raw() is color.raw()
    assert MatrixOS_ColorEffects.color_triangle(color).raw() is color.raw()

    assert effect_calls[0] == ("rainbow", 1000, 0)
    assert effect_calls[-1][0] == "color_triangle"
    assert MatrixOS_ColorEffects.ColorEffects.rainbow().raw().value == 1


def test_constant_modules_import_and_values():
    assert MatrixOS_Direction.Direction.UP == 0
    assert MatrixOS_Direction.Direction.RIGHT == 90
    assert MatrixOS_KeyState.KeyState.PRESSED == 2
    assert MatrixOS_KeyState.KeyState.RELEASED == 5
    assert MatrixOS_KeyboardKeycode.KeyboardKeycode.KEY_A == 4
    assert MatrixOS_MouseKeycode.MouseKeycode.MOUSE_LEFT == 1
    assert MatrixOS_ConsumerKeycode.ConsumerKeycode.MEDIA_PLAY_PAUSE == 0xCD
    assert MatrixOS_SystemKeycode.SystemKeycode.SYSTEM_SLEEP == 0x82
    assert MatrixOS_GamepadDPadDirection.GamepadDPadDirection.GAMEPAD_DPAD_RIGHT == 3
    assert MatrixOS_InputClass.UNKNOWN == 0
    assert MatrixOS_InputClass.KEYPAD == 1
    assert MatrixOS_InputClass.GENERIC == 9


def test_utils_facade():
    assert MatrixOS_Utils.string_hash("Matrix") == 6


def test_timer_pythonic_facade():
    sys_time["millis"] = 1000
    timer = MatrixOS_Framework.Timer()

    assert not timer.tick(10)
    sys_time["millis"] = 1009
    assert not hasattr(timer, "Tick")
    sys_time["millis"] = 1010
    assert timer.tick(10)
    assert timer.since_last_tick() == 0

    sys_time["millis"] = 1025
    assert timer.tick(10, True)
    assert timer.since_last_tick() == 5

    timer.record_current()
    assert timer.since_last_tick() == 0


def test_framework_aggregate_exports():
    assert MatrixOS_Framework.Direction.UP == 0
    assert MatrixOS_Framework.KeyState.PRESSED == 2
    assert MatrixOS_Framework.KeyboardKeycode.KEY_A == 4
    assert MatrixOS_Framework.MidiPacket is not None


if __name__ == "__main__":
    test_color_effect_lowercase_aliases()
    test_constant_modules_import_and_values()
    test_utils_facade()
    test_timer_pythonic_facade()
    test_framework_aggregate_exports()
