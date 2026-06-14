import _MatrixOS_Dimension


class Dimension:
    def __init__(self, width: int = 0, height: int = 0):
        self.native = _MatrixOS_Dimension.Dimension(width, height)

    def x(self) -> int:
        return self.native.X()

    def y(self) -> int:
        return self.native.Y()

    def width(self) -> int:
        return self.native.X()

    def height(self) -> int:
        return self.native.Y()

    def set_x(self, x: int) -> None:
        self.native.SetX(x)

    def set_y(self, y: int) -> None:
        self.native.SetY(y)

    def set_width(self, width: int) -> None:
        self.native.SetX(width)

    def set_height(self, height: int) -> None:
        self.native.SetY(height)

    def contains(self, point) -> bool:
        if hasattr(point, "native"):
            point = point.native
        return self.native.Contains(point)

    def area(self) -> int:
        return self.native.Area()

    def raw(self):
        return self.native


class DimensionView:
    def __init__(self, native = None):
        if native is None:
            native = Dimension(0, 0).native
        self.native = native

    def x(self) -> int:
        return self.native.X()

    def y(self) -> int:
        return self.native.Y()

    def width(self) -> int:
        return self.native.X()

    def height(self) -> int:
        return self.native.Y()

    def set_x(self, x: int) -> None:
        self.native.SetX(x)

    def set_y(self, y: int) -> None:
        self.native.SetY(y)

    def set_width(self, width: int) -> None:
        self.native.SetX(width)

    def set_height(self, height: int) -> None:
        self.native.SetY(height)

    def contains(self, point) -> bool:
        if hasattr(point, "native"):
            point = point.native
        return self.native.Contains(point)

    def area(self) -> int:
        return self.native.Area()

    def raw(self):
        return self.native

    def __bool__(self) -> bool:
        return bool(self.native)


def dimension(width: int, height: int):
    return Dimension(width, height)
