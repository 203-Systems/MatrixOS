import _MatrixOS_Input
from MatrixOS_InputId import InputId, InputIdView
from MatrixOS_InputEvent import InputEvent, InputEventView
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

def get_event(timeout_ms: int = 0) -> any:  # InputEvent or None
    event = GetEvent(timeout_ms)
    if event is None:
        return None
    return InputEventView(event)

def GetState(input_id: InputId) -> any:  # InputSnapshot or None
    if hasattr(input_id, "native"):
        input_id = input_id.native
    return _MatrixOS_Input.GetState(input_id)

def get_state(input_id: InputId) -> any:  # InputSnapshot or None
    if hasattr(input_id, "native"):
        input_id = input_id.native
    return GetState(input_id)

def GetPosition(input_id: InputId) -> any:  # Point or None
    if hasattr(input_id, "native"):
        input_id = input_id.native
    return _MatrixOS_Input.GetPosition(input_id)

def get_position(input_id: InputId) -> any:  # Point or None
    if hasattr(input_id, "native"):
        input_id = input_id.native
    return GetPosition(input_id)

def GetInputsAt(point: Point) -> list:  # list of InputId
    return _MatrixOS_Input.GetInputsAt(point)

def get_inputs_at(point: Point) -> list:  # list of InputId
    return GetInputsAt(point)

def ClearInputBuffer() -> None:
    _MatrixOS_Input.ClearInputBuffer()

def clear_input_buffer() -> None:
    ClearInputBuffer()

def FunctionKey() -> InputId:
    return _MatrixOS_Input.FunctionKey()

def function_key() -> InputId:
    return InputIdView(FunctionKey())

def GetClusters() -> list:  # list of InputCluster
    return _MatrixOS_Input.GetClusters()

def get_clusters() -> list:  # list of InputCluster
    return GetClusters()

def GetPrimaryGridCluster() -> any:  # InputCluster or None
    return _MatrixOS_Input.GetPrimaryGridCluster()

def get_primary_grid_cluster() -> any:  # InputCluster or None
    return GetPrimaryGridCluster()
