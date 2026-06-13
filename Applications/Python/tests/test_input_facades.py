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
        return 0

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
        return False

    def KeyActive(self):
        return True

    def Keypad(self):
        return NativeKeypadInfo()

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

point_module = types.ModuleType("MatrixOS_Point")
point_module.Point = object
sys.modules["MatrixOS_Point"] = point_module

snapshot_module = types.ModuleType("MatrixOS_InputSnapshot")
snapshot_module.InputSnapshot = object
sys.modules["MatrixOS_InputSnapshot"] = snapshot_module

cluster_module = types.ModuleType("MatrixOS_InputCluster")
cluster_module.InputCluster = object
sys.modules["MatrixOS_InputCluster"] = cluster_module

input_class_module = types.ModuleType("MatrixOS_InputClass")
input_class_module.KEYPAD = 1
sys.modules["MatrixOS_InputClass"] = input_class_module

seen_position_id = []
input_module = types.ModuleType("_MatrixOS_Input")
input_module.GetEvent = lambda timeout_ms: NativeInputEvent()
input_module.GetState = lambda input_id: None
input_module.GetPosition = lambda input_id: seen_position_id.append(input_id) or None
input_module.GetInputsAt = lambda point: []
input_module.ClearInputBuffer = lambda: None
input_module.FunctionKey = lambda: NativeInputId(0, 0)
input_module.GetClusters = lambda: []
input_module.GetPrimaryGridCluster = lambda: None
sys.modules["_MatrixOS_Input"] = input_module

import MatrixOS_Input


def test_get_event_returns_pythonic_proxy():
    event = MatrixOS_Input.get_event()

    assert event.input_class() == 1
    assert event.cluster_id() == 1
    assert event.member_id() == 7
    assert event.is_aftertouch()
    assert event.key_force() == 0.5
    assert event.key_value(1) == 0.75

    input_id = event.id()
    assert input_id.cluster_id() == 1
    assert input_id.member_id() == 7
    assert input_id.input_id() == 7
    assert input_id.raw() is input_id.native
    assert event.input_id() == 7

    keypad = event.keypad()
    assert keypad.state() == 4
    assert keypad.velocity() == 0.75
    assert keypad.active()
    assert keypad.raw() is keypad.native
    assert event.info().state() == 4
    assert event.raw() is event.native


def test_input_functions_unwrap_proxy_ids():
    input_id = MatrixOS_Input.function_key()

    MatrixOS_Input.get_position(input_id)

    assert seen_position_id[-1] is input_id.native


def test_input_id_static_factories_return_proxies():
    function_key = MatrixOS_Input.InputIdView(NativeInputId.FunctionKey())
    invalid = MatrixOS_Input.InputIdView(NativeInputId.Invalid())

    assert function_key.is_function_key()
    assert invalid.cluster_id() == -1


if __name__ == "__main__":
    test_get_event_returns_pythonic_proxy()
    test_input_functions_unwrap_proxy_ids()
    test_input_id_static_factories_return_proxies()
