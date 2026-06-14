import _MatrixOS_InputCluster
from MatrixOS_Point import PointView
from MatrixOS_Dimension import DimensionView


class InputCluster:
    def __init__(self):
        self.native = _MatrixOS_InputCluster.InputCluster()

    def cluster_id(self) -> int:
        return self.native.ClusterId()

    def id(self) -> int:
        return self.native.ClusterId()

    def name(self) -> str:
        return self.native.Name()

    def input_class(self) -> int:
        return self.native.InputClass()

    def shape(self) -> int:
        return self.native.Shape()

    def root_point(self):
        return PointView(self.native.RootPoint())

    def dimension(self):
        return DimensionView(self.native.Dimension())

    def input_count(self) -> int:
        return self.native.InputCount()

    def has_root_point(self) -> bool:
        return self.native.HasRootPoint()

    def has_coordinates(self) -> bool:
        return self.native.HasCoordinates()

    def is_keypad(self) -> bool:
        return self.native.InputClass() == 1

    def is_grid(self) -> bool:
        return self.native.Shape() == 2

    def width(self) -> int:
        return self.native.Dimension().X()

    def height(self) -> int:
        return self.native.Dimension().Y()

    def raw(self):
        return self.native


class InputClusterView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_InputCluster.InputCluster()
        self.native = native

    def cluster_id(self) -> int:
        return self.native.ClusterId()

    def id(self) -> int:
        return self.native.ClusterId()

    def name(self) -> str:
        return self.native.Name()

    def input_class(self) -> int:
        return self.native.InputClass()

    def shape(self) -> int:
        return self.native.Shape()

    def root_point(self):
        return PointView(self.native.RootPoint())

    def dimension(self):
        return DimensionView(self.native.Dimension())

    def input_count(self) -> int:
        return self.native.InputCount()

    def has_root_point(self) -> bool:
        return self.native.HasRootPoint()

    def has_coordinates(self) -> bool:
        return self.native.HasCoordinates()

    def is_keypad(self) -> bool:
        return self.native.InputClass() == 1

    def is_grid(self) -> bool:
        return self.native.Shape() == 2

    def width(self) -> int:
        return self.native.Dimension().X()

    def height(self) -> int:
        return self.native.Dimension().Y()

    def raw(self):
        return self.native

    def __bool__(self) -> bool:
        return bool(self.native)
