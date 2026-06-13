import _MatrixOS_InputCluster

class InputCluster(_MatrixOS_InputCluster.InputCluster):
    def cluster_id(self) -> int:
        return self.ClusterId()

    def id(self) -> int:
        return self.ClusterId()

    def name(self) -> str:
        return self.Name()

    def input_class(self) -> int:
        return self.InputClass()

    def shape(self) -> int:
        return self.Shape()

    def root_point(self):
        return self.RootPoint()

    def dimension(self):
        return self.Dimension()

    def input_count(self) -> int:
        return self.InputCount()

    def has_root_point(self) -> bool:
        return self.HasRootPoint()

    def has_coordinates(self) -> bool:
        return self.HasCoordinates()

    def is_keypad(self) -> bool:
        return self.InputClass() == 1

    def is_grid(self) -> bool:
        return self.Shape() == 2

    def width(self) -> int:
        return self.Dimension().X()

    def height(self) -> int:
        return self.Dimension().Y()

    def raw(self):
        return self

class InputClusterView:
    def __init__(self, native = None):
        if native is None:
            native = _MatrixOS_InputCluster.InputCluster()
        self.native = native

    def ClusterId(self) -> int:
        return self.native.ClusterId()

    def cluster_id(self) -> int:
        return self.ClusterId()

    def id(self) -> int:
        return self.ClusterId()

    def Name(self) -> str:
        return self.native.Name()

    def name(self) -> str:
        return self.Name()

    def InputClass(self) -> int:
        return self.native.InputClass()

    def input_class(self) -> int:
        return self.InputClass()

    def Shape(self) -> int:
        return self.native.Shape()

    def shape(self) -> int:
        return self.Shape()

    def RootPoint(self):
        return self.native.RootPoint()

    def root_point(self):
        return self.RootPoint()

    def Dimension(self):
        return self.native.Dimension()

    def dimension(self):
        return self.Dimension()

    def InputCount(self) -> int:
        return self.native.InputCount()

    def input_count(self) -> int:
        return self.InputCount()

    def HasRootPoint(self) -> bool:
        return self.native.HasRootPoint()

    def has_root_point(self) -> bool:
        return self.HasRootPoint()

    def HasCoordinates(self) -> bool:
        return self.native.HasCoordinates()

    def has_coordinates(self) -> bool:
        return self.HasCoordinates()

    def is_keypad(self) -> bool:
        return self.InputClass() == 1

    def is_grid(self) -> bool:
        return self.Shape() == 2

    def width(self) -> int:
        return self.Dimension().X()

    def height(self) -> int:
        return self.Dimension().Y()

    def raw(self):
        return self.native

    def __bool__(self) -> bool:
        return bool(self.native)
