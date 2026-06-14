import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))


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

    def Contains(self, point):
        return point.X() >= 0 and point.Y() >= 0 and point.X() < self.x_value and point.Y() < self.y_value

    def Area(self):
        return self.x_value * self.y_value


class NativeInputCluster:
    def ClusterId(self):
        return 1

    def Name(self):
        return "Grid"

    def InputClass(self):
        return 1

    def Shape(self):
        return 2

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


class NativeInputId:
    def __init__(self, cluster=1, member=7):
        self.cluster = cluster
        self.member = member

    def ClusterId(self):
        return self.cluster

    def MemberId(self):
        return self.member

    def __bool__(self):
        return self.cluster >= 0


class NativeKeypadInfo:
    def State(self):
        return 2

    def Force(self):
        return 1.0

    def Value(self, index):
        return 0.75 if index == 1 else 1.0

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


class NativeInputSnapshot:
    def Id(self):
        return NativeInputId()

    def InputClass(self):
        return 1

    def Keypad(self):
        return NativeKeypadInfo()

    def __bool__(self):
        return True


point_module = types.ModuleType("_MatrixOS_Point")
point_module.Point = NativePoint
sys.modules["_MatrixOS_Point"] = point_module

dimension_module = types.ModuleType("_MatrixOS_Dimension")
dimension_module.Dimension = NativeDimension
sys.modules["_MatrixOS_Dimension"] = dimension_module

cluster_module = types.ModuleType("_MatrixOS_InputCluster")
cluster_module.InputCluster = NativeInputCluster
sys.modules["_MatrixOS_InputCluster"] = cluster_module

input_id_module = types.ModuleType("_MatrixOS_InputId")
input_id_module.InputId = NativeInputId
sys.modules["_MatrixOS_InputId"] = input_id_module

keypad_module = types.ModuleType("_MatrixOS_KeypadInfo")
keypad_module.KeypadInfo = NativeKeypadInfo
sys.modules["_MatrixOS_KeypadInfo"] = keypad_module

snapshot_module = types.ModuleType("_MatrixOS_InputSnapshot")
snapshot_module.InputSnapshot = NativeInputSnapshot
sys.modules["_MatrixOS_InputSnapshot"] = snapshot_module

import MatrixOS_Dimension
import MatrixOS_InputCluster
import MatrixOS_InputSnapshot
import MatrixOS_Point


def test_dimension_pythonic_aliases():
    dimension = MatrixOS_Dimension.dimension(8, 4)
    point = MatrixOS_Point.point(7, 3)

    assert dimension.x() == 8
    assert dimension.y() == 4
    assert dimension.width() == 8
    assert dimension.height() == 4
    assert dimension.contains(point)
    assert dimension.area() == 32

    dimension.set_width(10)
    dimension.set_height(5)
    assert dimension.x() == 10
    assert dimension.y() == 5


def test_input_cluster_pythonic_aliases():
    cluster = MatrixOS_InputCluster.InputCluster()

    assert cluster.cluster_id() == 1
    assert cluster.id() == 1
    assert cluster.name() == "Grid"
    assert cluster.is_keypad()
    assert cluster.is_grid()
    assert cluster.root_point().x() == 0
    assert cluster.has_root_point()
    assert cluster.width() == 8
    assert cluster.height() == 8
    assert cluster.input_count() == 64
    assert cluster.has_coordinates()

    view = MatrixOS_InputCluster.InputClusterView(NativeInputCluster())
    assert view.cluster_id() == 1
    assert view.id() == 1
    assert view.name() == "Grid"
    assert view.input_class() == 1
    assert view.shape() == 2
    assert view.root_point().y() == 0
    assert view.dimension().x() == 8
    assert view.dimension().y() == 8
    assert view.input_count() == 64
    assert view.has_root_point()
    assert view.has_coordinates()
    assert view.is_keypad()
    assert view.is_grid()
    assert view.width() == 8
    assert view.height() == 8
    assert view.raw().__class__ is NativeInputCluster
    assert bool(view)


def test_input_snapshot_view_wraps_payload():
    snapshot = MatrixOS_InputSnapshot.InputSnapshotView(NativeInputSnapshot())

    assert snapshot.id().member_id() == 7
    assert snapshot.input_class() == 1
    assert snapshot.info().state() == 2
    assert snapshot.keypad().velocity() == 0.75
    assert snapshot.raw().__class__ is NativeInputSnapshot


if __name__ == "__main__":
    test_dimension_pythonic_aliases()
    test_input_cluster_pythonic_aliases()
    test_input_snapshot_view_wraps_payload()
