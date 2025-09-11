import _MatrixOSPoint

class Point(_MatrixOSPoint.Point):
    def Rotate(self, rotation: int, dimension: 'Point', reverse: bool = False) -> 'Point':
        return super().Rotate(rotation, dimension, reverse)