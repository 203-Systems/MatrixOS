import _MatrixOS_Input
from MatrixOS_InputId import InputId, InputIdView
from MatrixOS_InputEvent import InputEvent, InputEventView
from MatrixOS_InputSnapshot import InputSnapshot, InputSnapshotView
from MatrixOS_KeypadInfo import KeypadInfo
from MatrixOS_InputCluster import InputCluster, InputClusterView
from MatrixOS_Point import Point, PointView
import MatrixOS_InputClass as InputClass


def _native(value):
    if hasattr(value, "native"):
        return value.native
    return value


def _cluster_view(cluster):
    if cluster is None:
        return None
    if hasattr(cluster, "native"):
        return cluster
    return InputClusterView(cluster)


def get_event(timeout_ms: int = 0) -> any:
    event = _MatrixOS_Input.GetEvent(timeout_ms)
    if event is None:
        return None
    return InputEventView(event)


def get_state(input_id: InputId) -> any:
    snapshot = _MatrixOS_Input.GetState(_native(input_id))
    if snapshot is None:
        return None
    return InputSnapshotView(snapshot)


def get_position(input_id: InputId) -> any:
    point = _MatrixOS_Input.GetPosition(_native(input_id))
    if point is None:
        return None
    if hasattr(point, "native"):
        return point
    return PointView(point)


def try_get_point(input_id: InputId) -> any:
    return get_position(input_id)


def get_inputs_at(point: Point) -> list:
    result = []
    for input_id in _MatrixOS_Input.GetInputsAt(_native(point)):
        result.append(InputIdView(input_id))
    return result


def clear_input_buffer() -> None:
    _MatrixOS_Input.ClearInputBuffer()


def function_key() -> InputId:
    return InputIdView(_MatrixOS_Input.FunctionKey())


def get_clusters() -> list:
    result = []
    for cluster in _MatrixOS_Input.GetClusters():
        result.append(_cluster_view(cluster))
    return result


def clusters() -> list:
    return get_clusters()


def get_cluster(target) -> any:
    for cluster in get_clusters():
        if cluster.cluster_id() == target or cluster.name() == target:
            return cluster
    return None


def get_cluster_name(cluster_id: int) -> str:
    cluster = get_cluster(cluster_id)
    if cluster is None:
        return ""
    return cluster.name()


def get_keypad_clusters() -> list:
    result = []
    for cluster in get_clusters():
        if cluster.input_class() == InputClass.KEYPAD:
            result.append(cluster)
    return result


def keypad_clusters() -> list:
    return get_keypad_clusters()


def get_keypad_cluster(name: str = "") -> any:
    for cluster in get_keypad_clusters():
        if name == "" or cluster.name() == name:
            return cluster
    return None


def keypad_cluster(name: str = "") -> any:
    return get_keypad_cluster(name)


def get_primary_grid_cluster() -> any:
    return _cluster_view(_MatrixOS_Input.GetPrimaryGridCluster())


def primary_grid() -> any:
    return get_primary_grid_cluster()
