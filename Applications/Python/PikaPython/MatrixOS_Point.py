import _MatrixOS_Point


def _native(point):
    if hasattr(point, "native"):
        return point.native
    return point


class Point:
    def __init__(self, x: int = 0, y: int = 0):
        self.native = _MatrixOS_Point.Point(x, y)

    def x(self) -> int:
        return self.native.X()

    def y(self) -> int:
        return self.native.Y()

    def set_x(self, x: int) -> None:
        self.native.SetX(x)

    def set_y(self, y: int) -> None:
        self.native.SetY(y)

    def rotate(self, rotation: int, dimension, reverse: bool = False):
        if hasattr(dimension, "native"):
            dimension = dimension.native
        return PointView(self.native.Rotate(rotation, dimension, reverse))

    def raw(self):
        return self.native


def point(x: int, y: int):
    return Point(x, y)


class PointView:
    def __init__(self, native = None):
        if native is None:
            native = Point(0, 0).native
        self.native = native

    def x(self) -> int:
        return self.native.X()

    def y(self) -> int:
        return self.native.Y()

    def set_x(self, x: int) -> None:
        self.native.SetX(x)

    def set_y(self, y: int) -> None:
        self.native.SetY(y)

    def raw(self):
        return self.native

    def __bool__(self) -> bool:
        return bool(self.native)
