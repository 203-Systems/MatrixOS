import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))


class NativeInputId:
    def __init__(self, cluster=1, member=7):
        self.cluster = cluster
        self.member = member

    def ClusterId(self):
        return self.cluster

    def MemberId(self):
        return self.member

    def __eq__(self, other):
        return self.cluster == other.cluster and self.member == other.member

    def __bool__(self):
        return self.cluster >= 0

    def FunctionKey():
        return NativeInputId(0, 0)

    def Invalid():
        return NativeInputId(-1, -1)


class NativePoint:
    def __init__(self, x=0, y=0):
        self.x_value = x
        self.y_value = y

    def X(self):
        return self.x_value

    def Y(self):
        return self.y_value

    def SetX(self, value):
        self.x_value = value

    def SetY(self, value):
        self.y_value = value


class NativeDimension:
    def __init__(self, x=8, y=8):
        self.x_value = x
        self.y_value = y

    def X(self):
        return self.x_value

    def Y(self):
        return self.y_value

    def SetX(self, value):
        self.x_value = value

    def SetY(self, value):
        self.y_value = value

    def Contains(self, point):
        return True

    def Area(self):
        return self.x_value * self.y_value

    def __bool__(self):
        return True


class NativeKeypadInfo:
    def State(self):
        return 4

    def Force(self):
        return 0.5

    def Value(self, index):
        return 0.75 if index == 1 else 0.5

    def LastEventTime(self):
        return 123

    def Hold(self):
        return False

    def Active(self):
        return True

    def HoldTime(self):
        return 44

    def __bool__(self):
        return True


class NativeInputEvent:
    def Id(self):
        return NativeInputId()

    def InputClass(self):
        return 1

    def ClusterId(self):
        return 1

    def MemberId(self):
        return 7

    def KeyState(self):
        return 4

    def KeyForce(self):
        return 0.5

    def KeyValue(self, index):
        return 0.75 if index == 1 else 0.5

    def KeyHold(self):
        return True

    def KeyActive(self):
        return True

    def Keypad(self):
        return NativeKeypadInfo()

    def __bool__(self):
        return True


class NativeInputSnapshot:
    def Id(self):
        return NativeInputId()

    def InputClass(self):
        return 1

    def Keypad(self):
        return NativeKeypadInfo()

    def __bool__(self):
        return True


class NativeInputCluster:
    def __init__(self, cluster=1, name="Grid", input_class=1, shape=2):
        self.cluster = cluster
        self.cluster_name = name
        self.input_class_value = input_class
        self.shape_value = shape

    def ClusterId(self):
        return self.cluster

    def Name(self):
        return self.cluster_name

    def InputClass(self):
        return self.input_class_value

    def Shape(self):
        return self.shape_value

    def RootPoint(self):
        return NativePoint(0, 0)

    def Dimension(self):
        return NativeDimension(8, 8)

    def InputCount(self):
        return 64

    def HasRootPoint(self):
        return True

    def HasCoordinates(self):
        return True

    def __bool__(self):
        return True


input_id_module = types.ModuleType("_MatrixOS_InputId")
input_id_module.InputId = NativeInputId
sys.modules["_MatrixOS_InputId"] = input_id_module

keypad_module = types.ModuleType("_MatrixOS_KeypadInfo")
keypad_module.KeypadInfo = NativeKeypadInfo
sys.modules["_MatrixOS_KeypadInfo"] = keypad_module

event_module = types.ModuleType("_MatrixOS_InputEvent")
event_module.InputEvent = NativeInputEvent
sys.modules["_MatrixOS_InputEvent"] = event_module

snapshot_native_module = types.ModuleType("_MatrixOS_InputSnapshot")
snapshot_native_module.InputSnapshot = NativeInputSnapshot
sys.modules["_MatrixOS_InputSnapshot"] = snapshot_native_module

cluster_native_module = types.ModuleType("_MatrixOS_InputCluster")
cluster_native_module.InputCluster = NativeInputCluster
sys.modules["_MatrixOS_InputCluster"] = cluster_native_module

point_module = types.ModuleType("_MatrixOS_Point")
point_module.Point = NativePoint
sys.modules["_MatrixOS_Point"] = point_module

dimension_module = types.ModuleType("_MatrixOS_Dimension")
dimension_module.Dimension = NativeDimension
sys.modules["_MatrixOS_Dimension"] = dimension_module

input_class_module = types.ModuleType("MatrixOS_InputClass")
input_class_module.KEYPAD = 1
sys.modules["MatrixOS_InputClass"] = input_class_module

seen_position_id = []
clear_calls = []
input_module = types.ModuleType("_MatrixOS_Input")
input_module.GetEvent = lambda timeout_ms: NativeInputEvent()
input_module.GetState = lambda input_id: NativeInputSnapshot()
input_module.GetPosition = lambda input_id: seen_position_id.append(input_id) or NativePoint(2, 3)
input_module.GetInputsAt = lambda point: [NativeInputId(1, 2)]
input_module.ClearInputBuffer = lambda: clear_calls.append(True)
input_module.FunctionKey = lambda: NativeInputId(0, 0)
input_module.GetClusters = lambda: [
    NativeInputCluster(0, "Function", 1, 0),
    NativeInputCluster(1, "Grid", 1, 2),
    NativeInputCluster(2, "Slider", 2, 1),
]
input_module.GetPrimaryGridCluster = lambda: NativeInputCluster(1, "Grid", 1, 2)
sys.modules["_MatrixOS_Input"] = input_module

import MatrixOS_Input
import MatrixOS_InputCluster
import MatrixOS_InputEvent
import MatrixOS_InputId
import MatrixOS_InputSnapshot
import MatrixOS_KeypadInfo


def test_get_event_returns_pythonic_proxy():
    event = MatrixOS_Input.get_event()

    assert isinstance(event, MatrixOS_InputEvent.InputEventView)
    assert event.input_class() == 1
    assert event.cluster_id() == 1
    assert event.member_id() == 7
    assert event.is_aftertouch()
    assert event.key_pressure() == 0.5
    assert event.key_velocity() == 0.75
    assert event.key_value(1) == 0.75
    assert event.key_hold()
    assert event.key_active()
    assert not event.is_hold()

    input_id = event.id()
    assert input_id.cluster_id() == 1
    assert input_id.member_id() == 7
    assert input_id.cluster_name() == "Grid"
    assert input_id.raw() is input_id.native
    assert event.cluster_name() == "Grid"
    assert not (input_id != MatrixOS_Input.InputIdView(NativeInputId(1, 7)))

    keypad = event.keypad()
    assert keypad.state() == 4
    assert keypad.velocity() == 0.75
    assert keypad.active()
    assert keypad.last_event_time() == 123
    assert keypad.hold_time() == 44
    assert keypad.raw() is keypad.native
    assert event.info().state() == 4
    assert event.raw() is event.native
    assert not event.is_function_key()

    class NativeFunctionEvent(NativeInputEvent):
        def Id(self):
            return NativeInputId.FunctionKey()

        def ClusterId(self):
            return 0

        def MemberId(self):
            return 0

    assert MatrixOS_Input.InputEventView(NativeFunctionEvent()).is_function_key()


def test_input_functions_unwrap_proxy_ids():
    input_id = MatrixOS_Input.function_key()

    MatrixOS_Input.get_position(input_id)
    assert MatrixOS_Input.get_inputs_at(object())[0].member_id() == 2
    MatrixOS_Input.clear_input_buffer()

    assert seen_position_id[-1] is input_id.native
    position = MatrixOS_Input.get_position(input_id)
    assert position.x() == 2
    assert position.y() == 3
    assert position.raw().X() == 2
    assert clear_calls[-1]

    snapshot = MatrixOS_Input.get_state(input_id)
    assert snapshot.id().member_id() == 7
    assert snapshot.info().state() == 4

    original_get_state = input_module.GetState
    original_get_position = input_module.GetPosition
    try:
        input_module.GetState = lambda native_id: None
        input_module.GetPosition = lambda native_id: seen_position_id.append(native_id) or None
        assert MatrixOS_Input.get_state(input_id) is None
        MatrixOS_Input.get_position(input_id)
        assert seen_position_id[-1] is input_id.native
    finally:
        input_module.GetState = original_get_state
        input_module.GetPosition = original_get_position


def test_input_id_static_factories_return_proxies():
    function_key = MatrixOS_Input.InputIdView(NativeInputId.FunctionKey())
    invalid = MatrixOS_Input.InputIdView(NativeInputId.Invalid())

    assert function_key.is_function_key()
    assert invalid.cluster_id() == -1
    assert function_key.__ne__(invalid)


def test_native_wrapper_alias_methods():
    input_id = MatrixOS_InputId.InputId(1, 7)
    assert input_id.cluster_id() == 1
    assert input_id.member_id() == 7
    assert input_id.cluster_name() == "Grid"
    assert not input_id.is_function_key()
    assert input_id.raw() is input_id.native

    event = MatrixOS_InputEvent.InputEvent()
    assert event.id().member_id() == 7
    assert event.input_class() == 1
    assert event.cluster_id() == 1
    assert event.member_id() == 7
    assert event.cluster_name() == "Grid"
    assert event.key_state() == 4
    assert event.key_pressure() == 0.5
    assert event.key_velocity() == 0.75
    assert event.key_value(1) == 0.75
    assert event.key_hold()
    assert event.key_active()
    assert event.keypad().state() == 4
    assert event.info().state() == 4
    assert event.raw() is event.native
    assert event.is_aftertouch()
    assert not event.is_pressed()
    assert not event.is_hold()
    assert not event.is_released()

    keypad = MatrixOS_KeypadInfo.KeypadInfo()
    assert keypad.state() == 4
    assert keypad.force() == 0.5
    assert keypad.value(1) == 0.75
    assert keypad.velocity() == 0.75
    assert keypad.last_event_time() == 123
    assert not keypad.hold()
    assert keypad.active()
    assert keypad.hold_time() == 44
    assert keypad.raw() is keypad.native

    keypad_view = MatrixOS_KeypadInfo.KeypadInfoView()
    assert keypad_view.state() == 4
    assert keypad_view.force() == 0.5
    assert keypad_view.value(1) == 0.75
    assert keypad_view.last_event_time() == 123
    assert keypad_view.hold() is False
    assert keypad_view.active()
    assert keypad_view.hold_time() == 44
    assert bool(keypad_view)

    snapshot = MatrixOS_InputSnapshot.InputSnapshot()
    assert snapshot.id().member_id() == 7
    assert snapshot.input_class() == 1
    assert snapshot.keypad().state() == 4
    assert snapshot.info().state() == 4
    assert snapshot.raw() is snapshot.native

    cluster = MatrixOS_InputCluster.InputCluster()
    assert cluster.cluster_id() == 1
    assert cluster.id() == 1
    assert cluster.name() == "Grid"
    assert cluster.input_class() == 1
    assert cluster.shape() == 2
    assert cluster.is_keypad()
    assert cluster.is_grid()
    assert cluster.raw() is cluster.native


def test_input_cluster_helpers():
    assert MatrixOS_Input._cluster_view(None) is None
    cluster_view = MatrixOS_InputCluster.InputClusterView(NativeInputCluster(4, "View", 1, 2))
    assert MatrixOS_Input._cluster_view(cluster_view) is cluster_view
    assert len(MatrixOS_Input.get_clusters()) == 3
    assert len(MatrixOS_Input.clusters()) == 3
    assert MatrixOS_Input.get_cluster("Grid").cluster_id() == 1
    assert MatrixOS_Input.get_cluster(2).name() == "Slider"
    assert MatrixOS_Input.get_cluster("Missing") is None
    assert MatrixOS_Input.get_keypad_cluster().name() == "Function"
    assert MatrixOS_Input.get_keypad_cluster("Grid").cluster_id() == 1
    assert MatrixOS_Input.keypad_cluster("Missing") is None
    assert len(MatrixOS_Input.get_keypad_clusters()) == 2
    assert len(MatrixOS_Input.keypad_clusters()) == 2
    assert MatrixOS_Input.primary_grid().name() == "Grid"
    assert MatrixOS_Input.get_primary_grid_cluster().name() == "Grid"

    original_get_event = input_module.GetEvent
    try:
        input_module.GetEvent = lambda timeout_ms: None
        assert MatrixOS_Input.get_event() is None
    finally:
        input_module.GetEvent = original_get_event


if __name__ == "__main__":
    test_get_event_returns_pythonic_proxy()
    test_input_functions_unwrap_proxy_ids()
    test_input_id_static_factories_return_proxies()
    test_native_wrapper_alias_methods()
    test_input_cluster_helpers()
