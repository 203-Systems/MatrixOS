import _MatrixOS_Color

class Color(_MatrixOS_Color.Color):
    def Dim(self, factor: float = 0.2168) -> Color:
        return super().Dim(factor)
    
    def DimIfNot(self, not_dim: bool, factor: float = 0.2168) -> Color:
        return super().DimIfNot(not_dim, factor)