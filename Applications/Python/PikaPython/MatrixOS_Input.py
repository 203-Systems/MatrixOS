import _MatrixOS_Input
from MatrixOS_InputId import InputId, InputIdView
from MatrixOS_InputEvent import InputEvent, InputEventView
from MatrixOS_InputSnapshot import InputSnapshot, InputSnapshotView
from MatrixOS_KeypadInfo import KeypadInfo
from MatrixOS_InputCluster import InputCluster, InputClusterView
from MatrixOS_Point import Point
import MatrixOS_InputClass as InputClass

# Nullable convention: PikaPython has no Optional[T].
# Functions that may return None use `-> any` with a comment
# documenting the real return type.

def _cluster_view(cluster):
    if cluster is None:
        return None
    if hasattr(cluster, "native"):
        return cluster
    return InputClusterView(cluster)

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
    snapshot = GetState(input_id)
    if snapshot is None:
        return None
    return InputSnapshotView(snapshot)

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
    result = []
    for cluster in GetClusters():
        result.append(_cluster_view(cluster))
    return result

def clusters() -> list:
    return get_clusters()

def GetCluster(target) -> any:
    for cluster in GetClusters():
        if cluster.ClusterId() == target or cluster.Name() == target:
            return _cluster_view(cluster)
    return None

def get_cluster(target) -> any:
    return GetCluster(target)

def GetKeypadClusters() -> list:
    result = []
    for cluster in GetClusters():
        if cluster.InputClass() == InputClass.KEYPAD:
            result.append(_cluster_view(cluster))
    return result

def get_keypad_clusters() -> list:
    return GetKeypadClusters()

def keypad_clusters() -> list:
    return GetKeypadClusters()

def GetKeypadCluster(name: str = "") -> any:
    for cluster in GetKeypadClusters():
        if name == "" or cluster.Name() == name:
            return cluster
    return None

def get_keypad_cluster(name: str = "") -> any:
    return GetKeypadCluster(name)

def keypad_cluster(name: str = "") -> any:
    return GetKeypadCluster(name)

def GetPrimaryGridCluster() -> any:  # InputCluster or None
    return _MatrixOS_Input.GetPrimaryGridCluster()

def get_primary_grid_cluster() -> any:  # InputCluster or None
    return _cluster_view(GetPrimaryGridCluster())

def primary_grid() -> any:
    return get_primary_grid_cluster()
