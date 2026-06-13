import _MatrixOS_LED
from MatrixOS_Point import Point
from MatrixOS_Color import Color

# Constants
CROSSFADE_DURATION = 200
CURRENT_LAYER = 255 # As the API will write to the current layer by default

def NextBrightness() -> None:
    _MatrixOS_LED.NextBrightness()

def next_brightness() -> None:
    NextBrightness()

def SetBrightness(brightness: int) -> None:
    _MatrixOS_LED.SetBrightness(brightness)

def set_brightness(brightness: int) -> None:
    SetBrightness(brightness)

def SetBrightnessMultiplier(partition_name: str, multiplier: float) -> None:
    _MatrixOS_LED.SetBrightnessMultiplier(partition_name, multiplier)

def set_brightness_multiplier(partition_name: str, multiplier: float) -> None:
    SetBrightnessMultiplier(partition_name, multiplier)

def SetColor(xy: Point, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.SetColor(xy, color, layer)

def set_color(xy: Point, color: Color, layer: int = CURRENT_LAYER) -> None:
    SetColor(xy, color, layer)

def set_color_xy(x: int, y: int, color: Color, layer: int = CURRENT_LAYER) -> None:
    SetColor(Point(x, y), color, layer)

def SetColorByID(id: int, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.SetColorByID(id, color, layer)

def set_color_by_id(id: int, color: Color, layer: int = CURRENT_LAYER) -> None:
    SetColorByID(id, color, layer)

def set_pixel(id: int, color: Color, layer: int = CURRENT_LAYER) -> None:
    SetColorByID(id, color, layer)

def Fill(color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.Fill(color, layer)

def fill(color: Color, layer: int = CURRENT_LAYER) -> None:
    Fill(color, layer)

def clear(layer: int = CURRENT_LAYER) -> None:
    Fill(Color(0), layer)

def FillPartition(partition: str, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.FillPartition(partition, color, layer)

def fill_partition(partition: str, color: Color, layer: int = CURRENT_LAYER) -> None:
    FillPartition(partition, color, layer)

def Update(layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.Update(layer)

def update(layer: int = CURRENT_LAYER) -> None:
    Update(layer)

def show(layer: int = CURRENT_LAYER) -> None:
    Update(layer)

def CurrentLayer() -> int:
    return _MatrixOS_LED.CurrentLayer()

def current_layer() -> int:
    return CurrentLayer()

def CreateLayer(crossfade: int = CROSSFADE_DURATION) -> int:
    return _MatrixOS_LED.CreateLayer(crossfade)

def create_layer(crossfade: int = CROSSFADE_DURATION) -> int:
    return CreateLayer(crossfade)

def CopyLayer(dest: int, src: int) -> None:
    _MatrixOS_LED.CopyLayer(dest, src)

def copy_layer(dest: int, src: int) -> None:
    CopyLayer(dest, src)

def DestroyLayer(crossfade: int = CROSSFADE_DURATION) -> bool:
    return _MatrixOS_LED.DestroyLayer(crossfade)

def destroy_layer(crossfade: int = CROSSFADE_DURATION) -> bool:
    return DestroyLayer(crossfade)

def Fade(crossfade: int = CROSSFADE_DURATION) -> None:
    _MatrixOS_LED.Fade(crossfade)

def fade(crossfade: int = CROSSFADE_DURATION) -> None:
    Fade(crossfade)

def PauseUpdate(pause: bool = True) -> None:
    _MatrixOS_LED.PauseUpdate(pause)

def pause_update(pause: bool = True) -> None:
    PauseUpdate(pause)

def GetLEDCount() -> int:
    return _MatrixOS_LED.GetLEDCount()

def get_led_count() -> int:
    return GetLEDCount()

def count() -> int:
    return GetLEDCount()
