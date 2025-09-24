import _MatrixOS_KeyPad
from MatrixOS_KeyEvent import KeyEvent
from MatrixOS_KeyInfo import KeyInfo
from MatrixOS_Point import Point

FunctionKeyID:int = 0

def Get(timeout_ms: int = 0) -> KeyEvent:
    return _MatrixOS_KeyPad.Get(timeout_ms)

def GetKey(keyXY: Point) -> KeyInfo:
    return _MatrixOS_KeyPad.GetKey(keyXY)

def GetKeyByID(keyID: int) -> KeyInfo:
    return _MatrixOS_KeyPad.GetKeyByID(keyID)

def Clear() -> None:
    _MatrixOS_KeyPad.Clear()

def XY2ID(xy: Point) -> int:
    return _MatrixOS_KeyPad.XY2ID(xy)

def ID2XY(keyID: int) -> Point:
    return _MatrixOS_KeyPad.ID2XY(keyID)