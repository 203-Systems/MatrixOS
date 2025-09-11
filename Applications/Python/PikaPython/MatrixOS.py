import PikaStdLib

import _MatrixOS

from MatrixOSFramework import *

# Constants / Macro
CROSSFADE_DURATION = 200
CURRENT_LAYER = 255 # As the API will write to the current layer by default

# expose symbols
def Reboot() -> None:
    _MatrixOS.Reboot()

def Bootloader() -> None:
    _MatrixOS.Bootloader()

def DelayMs(ms: int) -> None:
    _MatrixOS.DelayMs(ms)

def Millis() -> int:
    return _MatrixOS.Millis()

def OpenSetting() -> None:
    _MatrixOS.OpenSetting()

def ExecuteAPP(author: str, app_name: str) -> None:
    _MatrixOS.ExecuteAPP(author, app_name)

def ExecuteAPP(app_id: int) -> None:
    _MatrixOS.ExecuteAPPByID(app_id)

class LED:
    def NextBrightness(self) -> None:
        _MatrixOS.LED.NextBrightness()

    def SetBrightness(self, brightness: int) -> None:
        _MatrixOS.LED.SetBrightness(brightness)

    def SetBrightnessMultiplier(self, partition_name: str, multiplier: float) -> None:
        _MatrixOS.LED.SetBrightnessMultiplier(partition_name, multiplier)

    def SetColor(self, xy: Point, color: Color, layer:int = CURRENT_LAYER) -> None:
        _MatrixOS.LED.SetColor(xy, color, layer)

    def SetColorByID(self, id: int, color: Color, layer:int = CURRENT_LAYER) -> None:
        _MatrixOS.LED.SetColorByID(id, color, layer)

    def Fill(self, color: Color, layer:int = CURRENT_LAYER) -> None:
        _MatrixOS.LED.Fill(color, layer)

    def FillPartition(self, partition: str, color: Color, layer:int = CURRENT_LAYER) -> None:
        _MatrixOS.LED.FillPartition(partition, color, layer)

    def Update(self, layer:int = CURRENT_LAYER) -> None:
        _MatrixOS.LED.Update(layer)

    def CurrentLayer(self) -> int:
        return _MatrixOS.LED.CurrentLayer()
    
    def CreateLayer(self, crossfade:int = CROSSFADE_DURATION) -> int:
        return _MatrixOS.LED.CreateLayer(crossfade)
    
    def CopyLayer(self, dest: int, src: int) -> None:
        _MatrixOS.LED.CopyLayer(dest, src)

    def DestroyLayer(self, crossfade:int = CROSSFADE_DURATION) -> bool:
        return _MatrixOS.LED.DestroyLayer(crossfade)

    def Fade(self, crossfade:int = CROSSFADE_DURATION) -> None:
        _MatrixOS.LED.Fade(crossfade)

    def PauseUpdate(self, pause: bool = True) -> None:
        _MatrixOS.LED.PauseUpdate(pause)

    def GetLedCount(self) -> int:
        return _MatrixOS.LED.GetLedCount()

class KeyPad:
    def Get(self, timeout_ms: int = 0) -> KeyEvent: 
        return _MatrixOS.KeyPad.Get(timeout_ms)
    
    def GetKey(self, keyXY: Point) -> KeyInfo:
        return _MatrixOS.KeyPad.GetKey(keyXY)
    
    def GetKeyByID(self, keyID: int) -> KeyInfo:
        return _MatrixOS.KeyPad.GetKeyByID(keyID)
    
    def Clear(self) -> None:
        _MatrixOS.KeyPad.Clear()

    def XY2ID(self, xy: Point) -> int:
        return _MatrixOS.KeyPad.XY2ID(xy)
    
    def ID2XY(self, keyID: int) -> Point:
        return _MatrixOS.KeyPad.ID2XY(keyID)
    