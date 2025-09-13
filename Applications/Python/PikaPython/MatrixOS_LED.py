import _MatrixOS_LED
from MatrixOS_Point import Point
from MatrixOS_Color import Color

# Constants
CROSSFADE_DURATION = 200
CURRENT_LAYER = 255 # As the API will write to the current layer by default

def NextBrightness() -> None:
    _MatrixOS_LED.NextBrightness()

def SetBrightness(brightness: int) -> None:
    _MatrixOS_LED.SetBrightness(brightness)

def SetBrightnessMultiplier(partition_name: str, multiplier: float) -> None:
    _MatrixOS_LED.SetBrightnessMultiplier(partition_name, multiplier)

def SetColor(xy: Point, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.SetColor(xy, color, layer)

def SetColorByID(id: int, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.SetColorByID(id, color, layer)

def Fill(color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.Fill(color, layer)

def FillPartition(partition: str, color: Color, layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.FillPartition(partition, color, layer)

def Update(layer: int = CURRENT_LAYER) -> None:
    _MatrixOS_LED.Update(layer)

def CurrentLayer() -> int:
    return _MatrixOS_LED.CurrentLayer()

def CreateLayer(crossfade: int = CROSSFADE_DURATION) -> int:
    return _MatrixOS_LED.CreateLayer(crossfade)

def CopyLayer(dest: int, src: int) -> None:
    _MatrixOS_LED.CopyLayer(dest, src)

def DestroyLayer(crossfade: int = CROSSFADE_DURATION) -> bool:
    return _MatrixOS_LED.DestroyLayer(crossfade)

def Fade(crossfade: int = CROSSFADE_DURATION) -> None:
    _MatrixOS_LED.Fade(crossfade)

def PauseUpdate(pause: bool = True) -> None:
    _MatrixOS_LED.PauseUpdate(pause)

def GetLEDCount() -> int:
    return _MatrixOS_LED.GetLEDCount()