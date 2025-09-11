import PikaStdLib

import _MatrixOS

from MatrixOS_Framework import *

# Constants / Macro
CROSSFADE_DURATION = 200
CURRENT_LAYER = 255 # As the API will write to the current layer by default

# expose symbols
class SYS:
    def Reboot() -> None:
        _MatrixOS.SYS.Reboot()

    def Bootloader() -> None:
        _MatrixOS.SYS.Bootloader()

    def DelayMs(ms: int) -> None:
        _MatrixOS.SYS.DelayMs(ms)

    def Millis() -> int:
        return _MatrixOS.SYS.Millis()

    def OpenSetting() -> None:
        _MatrixOS.SYS.OpenSetting()

    def ExecuteAPP(author: str, app_name: str) -> None:
        _MatrixOS.SYS.ExecuteAPP(author, app_name)

    def ExecuteAPP(app_id: int) -> None:
        _MatrixOS.SYS.ExecuteAPPByID(app_id)

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

class MIDI:
    def Get(self, timeout_ms: int = 0):
        return _MatrixOS.MIDI.Get(timeout_ms)
    
    def Send(self, packet, timeout_ms: int = 0) -> bool:
        return _MatrixOS.MIDI.Send(packet, timeout_ms)
    
    def SendSysEx(self, port: int, length: int, data: bytes, include_meta: bool = False) -> bool:
        return _MatrixOS.MIDI.SendSysEx(port, length, data, include_meta)

class NVS:
    def GetSize(self, hash: int) -> int:
        return _MatrixOS.NVS.GetSize(hash)
    
    def GetVariable(self, hash: int, length: int) -> bytes:
        return _MatrixOS.NVS.GetVariable(hash, length)
    
    def SetVariable(self, hash: int, data: bytes, length: int) -> bool:
        return _MatrixOS.NVS.SetVariable(hash, data, length)
    
    def DeleteVariable(self, hash: int) -> bool:
        return _MatrixOS.NVS.DeleteVariable(hash)

class HID:
    def Ready() -> bool:
        return _MatrixOS.HID.Ready()
    
    class Keyboard:
        def Write(keycode: int) -> bool:
            return _MatrixOS.HID.Keyboard.Write(keycode)
        
        def Press(keycode: int) -> bool:
            return _MatrixOS.HID.Keyboard.Press(keycode)
        
        def Release(keycode: int) -> bool:
            return _MatrixOS.HID.Keyboard.Release(keycode)
        
        def ReleaseAll() -> None:
            _MatrixOS.HID.Keyboard.ReleaseAll()
    
    class Gamepad:
        def Press(button_id: int) -> None:
            _MatrixOS.HID.Gamepad.Press(button_id)
        
        def Release(button_id: int) -> None:
            _MatrixOS.HID.Gamepad.Release(button_id)
        
        def ReleaseAll() -> None:
            _MatrixOS.HID.Gamepad.ReleaseAll()
        
        def Button(button_id: int, state: bool) -> None:
            _MatrixOS.HID.Gamepad.Button(button_id, state)
        
        def Buttons(button_mask: int) -> None:
            _MatrixOS.HID.Gamepad.Buttons(button_mask)
        
        def XAxis(value: int) -> None:
            _MatrixOS.HID.Gamepad.XAxis(value)
        
        def YAxis(value: int) -> None:
            _MatrixOS.HID.Gamepad.YAxis(value)
        
        def ZAxis(value: int) -> None:
            _MatrixOS.HID.Gamepad.ZAxis(value)
        
        def RXAxis(value: int) -> None:
            _MatrixOS.HID.Gamepad.RXAxis(value)
        
        def RYAxis(value: int) -> None:
            _MatrixOS.HID.Gamepad.RYAxis(value)
        
        def RZAxis(value: int) -> None:
            _MatrixOS.HID.Gamepad.RZAxis(value)
        
        def DPad(direction: int) -> None:
            _MatrixOS.HID.Gamepad.DPad(direction)
    
    class RawHID:
        def Get(timeout_ms: int = 0) -> bytes:
            return _MatrixOS.HID.RawHID.Get(timeout_ms)
        
        def Send(report: bytes) -> bool:
            return _MatrixOS.HID.RawHID.Send(report)

class USB:
    def Connected() -> bool:
        return _MatrixOS.USB.Connected()
    
    class CDC:
        def Connected() -> bool:
            return _MatrixOS.USB.CDC.Connected()
        
        def Available() -> int:
            return _MatrixOS.USB.CDC.Available()
        
        def Poll() -> None:
            _MatrixOS.USB.CDC.Poll()
        
        def Print(text: str, end: str = "\n") -> None:
            _MatrixOS.USB.CDC.Print(text, end)
        
        def Flush() -> None:
            _MatrixOS.USB.CDC.Flush()
        
        def Read() -> int:
            return _MatrixOS.USB.CDC.Read()
        
        def ReadBytes(length: int) -> bytes:
            return _MatrixOS.USB.CDC.ReadBytes(length)
        
        def ReadString() -> str:
            return _MatrixOS.USB.CDC.ReadString()