import _MatrixOS_Input
from MatrixOS_InputId import InputId
from MatrixOS_InputEvent import InputEvent
from MatrixOS_InputSnapshot import InputSnapshot
from MatrixOS_KeypadInfo import KeypadInfo
from MatrixOS_InputCluster import InputCluster
from MatrixOS_Point import Point
import MatrixOS_InputClass as InputClass

# Nullable convention: PikaPython has no Optional[T].
# Functions that may return None use `-> any` with a comment
# documenting the real return type.

def GetEvent(timeout_ms: int = 0) -> any:  # InputEvent or None
    return _MatrixOS_Input.GetEvent(timeout_ms)

def GetState(input_id: InputId) -> any:  # InputSnapshot or None
    return _MatrixOS_Input.GetState(input_id)

def GetPosition(input_id: InputId) -> any:  # Point or None
    return _MatrixOS_Input.GetPosition(input_id)

def GetInputsAt(point: Point) -> list:  # list of InputId
    return _MatrixOS_Input.GetInputsAt(point)

def ClearInputBuffer() -> None:
    _MatrixOS_Input.ClearInputBuffer()

def FunctionKey() -> InputId:
    return _MatrixOS_Input.FunctionKey()

def GetClusters() -> list:  # list of InputCluster
    return _MatrixOS_Input.GetClusters()

def GetPrimaryGridCluster() -> any:  # InputCluster or None
    return _MatrixOS_Input.GetPrimaryGridCluster()
