import _MatrixOS_LED
from MatrixOS_Point import Point
from MatrixOS_Color import Color


CROSSFADE_DURATION = 200
CURRENT_LAYER = 255
LED_TYPE_MONO_1B = 0x01
LED_TYPE_MONO_8B = 0x08
LED_TYPE_RGB_24B = 0x10
LED_TYPE_RGBW_32B_6K5 = 0x20


class LEDPartition:
    def __init__(self, index: int, name: str, start: int, size: int, led_type: int, default_multiplier: float):
        self.index = index
        self.name = name
        self.start = start
        self.size = size
        self.type = led_type
        self.default_multiplier = default_multiplier

    def end(self) -> int:
        return self.start + self.size

    def contains(self, led_id: int) -> bool:
        return led_id >= self.start and led_id < self.end()

    def is_rgb(self) -> bool:
        return self.type == LED_TYPE_RGB_24B

    def is_rgbw(self) -> bool:
        return self.type == LED_TYPE_RGBW_32B_6K5


def _native(value):
    if hasattr(value, "native"):
        return value.native
    return value


def next_brightness() -> None:
    _MatrixOS_LED.NextBrightness()


def set_brightness(brightness: int) -> None:
    _MatrixOS_LED.SetBrightness(brightness)


def set_brightness_multiplier(partition_name: str, multiplier: float) -> bool:
    return _MatrixOS_LED.SetBrightnessMultiplier(partition_name, multiplier)


def set_color(xy: Point, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.SetColor(_native(xy), _native(color), layer)


def set_color_xy(x: int, y: int, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.SetColor(Point(x, y).native, _native(color), layer)


def set_color_by_id(led_id: int, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.SetColorByID(led_id, _native(color), layer)


def set_pixel(led_id: int, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.SetColorByID(led_id, _native(color), layer)


def fill(color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.Fill(_native(color), layer)


def clear(layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.Fill(Color(0).native, layer)


def fill_partition(partition: str, color: Color, layer: int = CURRENT_LAYER) -> bool:
    return _MatrixOS_LED.FillPartition(partition, _native(color), layer)


def update(layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.Update(layer)


def show(layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.Update(layer)


def current_layer() -> int:
    return _MatrixOS_LED.CurrentLayer()


def create_layer(crossfade: int = CROSSFADE_DURATION) -> int:
    return _MatrixOS_LED.CreateLayer(crossfade)


def copy_layer(dest: int, src: int) -> None:
    _MatrixOS_LED.CopyLayer(dest, src)


def destroy_layer(crossfade: int = CROSSFADE_DURATION) -> bool:
    return _MatrixOS_LED.DestroyLayer(crossfade)


def fade(crossfade: int = CROSSFADE_DURATION) -> None:
    _MatrixOS_LED.Fade(crossfade)


def pause_update(pause: bool = True) -> None:
    _MatrixOS_LED.PauseUpdate(pause)


def get_led_count() -> int:
    return _MatrixOS_LED.GetLEDCount()


def count() -> int:
    return get_led_count()


def partition_count() -> int:
    return _MatrixOS_LED.GetPartitionCount()


def get_partition(index: int):
    if index < 0 or index >= partition_count():
        return None
    return LEDPartition(
        index,
        _MatrixOS_LED.GetPartitionName(index),
        _MatrixOS_LED.GetPartitionStart(index),
        _MatrixOS_LED.GetPartitionSize(index),
        _MatrixOS_LED.GetPartitionType(index),
        _MatrixOS_LED.GetPartitionDefaultMultiplier(index),
    )


def get_partition_by_name(name: str):
    index = _MatrixOS_LED.GetPartitionIndex(name)
    if index < 0:
        return None
    return get_partition(index)


def get_partitions() -> list:
    result = []
    for index in range(partition_count()):
        result.append(get_partition(index))
    return result


def partitions() -> list:
    return get_partitions()
