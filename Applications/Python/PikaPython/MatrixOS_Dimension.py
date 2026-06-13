# Dimension wrapper for MatrixOS
# Binding of OS/Framework/Geometry/Dimension.h

import _MatrixOS_Dimension

class Dimension(_MatrixOS_Dimension.Dimension):
    """2D dimension structure containing width and height"""

    def x(self) -> int:
        return self.X()

    def y(self) -> int:
        return self.Y()

    def width(self) -> int:
        return self.X()

    def height(self) -> int:
        return self.Y()

    def set_x(self, x: int) -> None:
        self.SetX(x)

    def set_y(self, y: int) -> None:
        self.SetY(y)

    def set_width(self, width: int) -> None:
        self.SetX(width)

    def set_height(self, height: int) -> None:
        self.SetY(height)

    def contains(self, point) -> bool:
        return self.Contains(point)

    def area(self) -> int:
        return self.Area()

def dimension(width: int, height: int):
    return Dimension(width, height)
