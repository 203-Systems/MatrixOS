import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))


class NativeColor:
    def __init__(self, *values):
        if len(values) == 4:
            self.w_value = values[3]
            self.r_value = values[0]
            self.g_value = values[1]
            self.b_value = values[2]
        elif len(values) == 3:
            self.w_value = 0
            self.r_value = values[0]
            self.g_value = values[1]
            self.b_value = values[2]
        elif len(values) == 1:
            wrgb = values[0]
            self.w_value = (wrgb >> 24) & 0xFF
            self.r_value = (wrgb >> 16) & 0xFF
            self.g_value = (wrgb >> 8) & 0xFF
            self.b_value = wrgb & 0xFF
        else:
            self.w_value = 0
            self.r_value = 0
            self.g_value = 0
            self.b_value = 0

    def R(self):
        return self.r_value

    def G(self):
        return self.g_value

    def B(self):
        return self.b_value

    def W(self):
        return self.w_value

    def RGB(self):
        return (self.r_value << 16) | (self.g_value << 8) | self.b_value

    def WRGB(self):
        return (self.w_value << 24) | self.RGB()

    def SetR(self, value):
        self.r_value = value

    def SetG(self, value):
        self.g_value = value

    def SetB(self, value):
        self.b_value = value

    def SetW(self, value):
        self.w_value = value


class NativePoint:
    def __init__(self, x, y):
        self.x_value = x
        self.y_value = y

    def X(self):
        return self.x_value

    def Y(self):
        return self.y_value

    def SetX(self, value):
        self.x_value = value

    def SetY(self, value):
        self.y_value = value


color_module = types.ModuleType("_MatrixOS_Color")
color_module.Color = NativeColor
sys.modules["_MatrixOS_Color"] = color_module

point_module = types.ModuleType("_MatrixOS_Point")
point_module.Point = NativePoint
sys.modules["_MatrixOS_Point"] = point_module

led_calls = []
led_module = types.ModuleType("_MatrixOS_LED")
led_module.SetColor = lambda xy, color, layer: led_calls.append(("set_color", xy, color, layer))
led_module.SetColorByID = lambda led_id, color, layer: led_calls.append(("set_id", led_id, color, layer))
led_module.Fill = lambda color, layer: led_calls.append(("fill", color, layer))
led_module.Update = lambda layer: led_calls.append(("update", layer))
led_module.GetLEDCount = lambda: 64
led_module.NextBrightness = lambda: None
led_module.SetBrightness = lambda brightness: None
led_module.SetBrightnessMultiplier = lambda partition, multiplier: None
led_module.FillPartition = lambda partition, color, layer: None
led_module.CurrentLayer = lambda: 2
led_module.CreateLayer = lambda crossfade: 3
led_module.CopyLayer = lambda dest, src: None
led_module.DestroyLayer = lambda crossfade: True
led_module.Fade = lambda crossfade: None
led_module.PauseUpdate = lambda pause: None
sys.modules["_MatrixOS_LED"] = led_module

import MatrixOS_Color
import MatrixOS_LED
import MatrixOS_Point


def test_color_pythonic_aliases():
    color = MatrixOS_Color.rgbw(10, 20, 30, 40)

    assert color.r() == 10
    assert color.g() == 20
    assert color.b() == 30
    assert color.w() == 40
    assert color.rgb() == 0x0A141E
    assert color.wrgb() == 0x280A141E

    color.set_r(1)
    color.set_g(2)
    color.set_b(3)
    color.set_w(4)

    assert color.wrgb() == 0x04010203


def test_point_pythonic_aliases():
    point = MatrixOS_Point.point(3, 4)

    assert point.x() == 3
    assert point.y() == 4

    point.set_x(5)
    point.set_y(6)

    assert point.x() == 5
    assert point.y() == 6


def test_led_pythonic_aliases():
    led_calls.clear()
    color = MatrixOS_Color.rgb(1, 2, 3)

    MatrixOS_LED.set_color_xy(2, 3, color)
    MatrixOS_LED.set_pixel(7, color)
    MatrixOS_LED.clear()
    MatrixOS_LED.show()

    assert led_calls[0][0] == "set_color"
    assert led_calls[0][1].x() == 2
    assert led_calls[0][1].y() == 3
    assert led_calls[1] == ("set_id", 7, color, MatrixOS_LED.CURRENT_LAYER)
    assert led_calls[2][0] == "fill"
    assert led_calls[3] == ("update", MatrixOS_LED.CURRENT_LAYER)
    assert MatrixOS_LED.count() == 64


if __name__ == "__main__":
    test_color_pythonic_aliases()
    test_point_pythonic_aliases()
    test_led_pythonic_aliases()
