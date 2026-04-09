import _MatrixOS_Input
from MatrixOS_InputId import InputId
from MatrixOS_InputEvent import InputEvent
from MatrixOS_KeypadInfo import KeypadInfo
from MatrixOS_InputCluster import InputCluster
from MatrixOS_Point import Point
import MatrixOS_InputClass as InputClass

def GetEvent(timeout_ms: int = 0) -> InputEvent:
    return _MatrixOS_Input.GetEvent(timeout_ms)

def GetState(input_id: InputId) -> KeypadInfo:
    return _MatrixOS_Input.GetState(input_id)

def GetPosition(input_id: InputId) -> Point:
    return _MatrixOS_Input.GetPosition(input_id)

def GetInputsAt(point: Point) -> list:
    return _MatrixOS_Input.GetInputsAt(point)

def ClearQueue() -> None:
    _MatrixOS_Input.ClearQueue()

def ClearState() -> None:
    _MatrixOS_Input.ClearState()

def FunctionKey() -> InputId:
    return _MatrixOS_Input.FunctionKey()

def GetClusters() -> list:
    return _MatrixOS_Input.GetClusters()

def GetPrimaryGridCluster() -> InputCluster:
    return _MatrixOS_Input.GetPrimaryGridCluster()
