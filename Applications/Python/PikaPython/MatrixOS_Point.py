import _MatrixOS_Point

class Point(_MatrixOS_Point.Point):
    def x(self) -> int:
        return self.X()

    def y(self) -> int:
        return self.Y()

    def set_x(self, x: int) -> None:
        self.SetX(x)

    def set_y(self, y: int) -> None:
        self.SetY(y)

    def Rotate(self, rotation: int, dimension, reverse: bool = False):
        return super().Rotate(rotation, dimension, reverse)

    def rotate(self, rotation: int, dimension, reverse: bool = False):
        return self.Rotate(rotation, dimension, reverse)

def point(x: int, y: int):
    return Point(x, y)
