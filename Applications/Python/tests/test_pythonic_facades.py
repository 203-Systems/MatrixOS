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

    def Scale(self, factor):
        return NativeColor(int(self.r_value * factor), int(self.g_value * factor), int(self.b_value * factor), int(self.w_value * factor))

    def Dim(self, factor=0.2168):
        return self.Scale(factor)

    def DimIfNot(self, not_dim, factor=0.2168):
        if not_dim:
            return self
        return self.Dim(factor)


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

    def Rotate(self, direction, dimension, reverse=False):
        if direction == 90 and not reverse:
            return NativePoint(-self.y_value, self.x_value)
        return NativePoint(self.x_value, self.y_value)


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
led_module.NextBrightness = lambda: led_calls.append(("next_brightness",))
led_module.SetBrightness = lambda brightness: led_calls.append(("brightness", brightness))
led_module.SetBrightnessMultiplier = lambda partition, multiplier: led_calls.append(("brightness_multiplier", partition, multiplier)) or True
led_module.FillPartition = lambda partition, color, layer: led_calls.append(("fill_partition", partition, color, layer)) or True
led_module.CurrentLayer = lambda: 2
led_module.CreateLayer = lambda crossfade: led_calls.append(("create_layer", crossfade)) or 3
led_module.CopyLayer = lambda dest, src: led_calls.append(("copy_layer", dest, src))
led_module.DestroyLayer = lambda crossfade: led_calls.append(("destroy_layer", crossfade)) or True
led_module.Fade = lambda crossfade: led_calls.append(("fade", crossfade))
led_module.PauseUpdate = lambda pause: led_calls.append(("pause", pause))
led_module.GetPartitionCount = lambda: 2
led_module.GetPartitionName = lambda index: ["Grid", "Underglow"][index] if index in (0, 1) else ""
led_module.GetPartitionStart = lambda index: [0, 64][index] if index in (0, 1) else -1
led_module.GetPartitionSize = lambda index: [64, 32][index] if index in (0, 1) else 0
led_module.GetPartitionType = lambda index: MatrixOS_LED_TYPE_RGB_24B if index in (0, 1) else 0
led_module.GetPartitionDefaultMultiplier = lambda index: [1.0, 4.0][index] if index in (0, 1) else 0.0
led_module.GetPartitionIndex = lambda name: {"Grid": 0, "Underglow": 1}.get(name, -1)
sys.modules["_MatrixOS_LED"] = led_module

import MatrixOS_Color
import MatrixOS_LED
import MatrixOS_Point

MatrixOS_LED_TYPE_RGB_24B = 0x10


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

    scaled = color.scale(2)
    assert scaled.R() == 2
    assert MatrixOS_Color.hex(0x01020304).wrgb() == 0x01020304
    assert color.dim_if_not(True) is color
    assert color.DimIfNot(True) is color


def test_point_pythonic_aliases():
    point = MatrixOS_Point.point(3, 4)

    assert point.x() == 3
    assert point.y() == 4

    point.set_x(5)
    point.set_y(6)

    assert point.x() == 5
    assert point.y() == 6
    assert point.rotate(90, object()).X() == -6
    assert point.Rotate(90, object()).Y() == 5


def test_led_pythonic_aliases():
    led_calls.clear()
    color = MatrixOS_Color.rgb(1, 2, 3)

    MatrixOS_LED.set_color_xy(2, 3, color)
    MatrixOS_LED.set_color_by_id(8, color)
    MatrixOS_LED.set_pixel(7, color)
    MatrixOS_LED.clear()
    MatrixOS_LED.show()
    MatrixOS_LED.next_brightness()
    MatrixOS_LED.set_brightness(128)
    assert MatrixOS_LED.set_brightness_multiplier("Grid", 0.5)
    assert MatrixOS_LED.fill_partition("Grid", color)
    assert MatrixOS_LED.current_layer() == 2
    assert MatrixOS_LED.create_layer(100) == 3
    MatrixOS_LED.copy_layer(1, 0)
    assert MatrixOS_LED.destroy_layer(100)
    MatrixOS_LED.fade(50)
    MatrixOS_LED.pause_update(True)

    assert led_calls[0][0] == "set_color"
    assert led_calls[0][1].x() == 2
    assert led_calls[0][1].y() == 3
    assert led_calls[1] == ("set_id", 8, color, MatrixOS_LED.CURRENT_LAYER)
    assert led_calls[2] == ("set_id", 7, color, MatrixOS_LED.CURRENT_LAYER)
    assert led_calls[3][0] == "fill"
    assert led_calls[4] == ("update", MatrixOS_LED.CURRENT_LAYER)
    assert MatrixOS_LED.count() == 64
    assert MatrixOS_LED.get_led_count() == 64


def test_led_partition_facade():
    partitions = MatrixOS_LED.partitions()

    assert isinstance(partitions[0], MatrixOS_LED.LEDPartition)
    assert len(partitions) == 2
    assert partitions[0].name == "Grid"
    assert partitions[0].start == 0
    assert partitions[0].size == 64
    assert partitions[0].end() == 64
    assert partitions[0].contains(63)
    assert not partitions[0].contains(64)
    assert partitions[0].is_rgb()
    assert not partitions[0].is_rgbw()
    assert MatrixOS_LED.partition_count() == 2
    assert MatrixOS_LED.GetPartitions()[0].name == "Grid"
    assert MatrixOS_LED.get_partitions()[1].name == "Underglow"
    assert MatrixOS_LED.fill_partition(partitions[0].name, MatrixOS_Color.rgb(1, 2, 3))
    assert MatrixOS_LED.set_brightness_multiplier(partitions[0].name, 0.5)

    underglow = MatrixOS_LED.get_partition_by_name("Underglow")
    assert underglow.index == 1
    assert MatrixOS_LED.GetPartitionByName("Grid").index == 0
    assert MatrixOS_LED.get_partition_by_name("Missing") is None
    assert led_calls[-2][0] == "fill_partition"
    assert led_calls[-2][1] == "Grid"
    assert led_calls[-2][3] == MatrixOS_LED.CURRENT_LAYER
    assert led_calls[-1] == ("brightness_multiplier", "Grid", 0.5)


if __name__ == "__main__":
    test_color_pythonic_aliases()
    test_point_pythonic_aliases()
    test_led_pythonic_aliases()
    test_led_partition_facade()
