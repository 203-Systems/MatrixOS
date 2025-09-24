import _MatrixOS_Point

class Point(_MatrixOS_Point.Point):
    def Rotate(self, rotation: int, dimension: Point, reverse: bool = False) -> Point:
        return super().Rotate(rotation, dimension, reverse)