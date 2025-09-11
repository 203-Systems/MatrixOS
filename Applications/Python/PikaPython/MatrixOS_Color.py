import _MatrixOS_Color

class Color(_MatrixOS_Color.Color):
    # def __init__(self, wrgb: int):
    #     r = (wrgb >> 16) & 0xFF
    #     g = (wrgb >> 8) & 0xFF
    #     b = wrgb & 0xFF
    #     w = (wrgb >> 24) & 0xFF
    #     super().__init__(r, g, b, w)

    def __init__(self, r: int, g: int, b: int, w: int = 0):
        super().__init__(r, g, b, w)
    
    def Dim(self, factor: float = 0.2168) -> 'Color':
        dimmed = super().Dim(factor)
        return Color(dimmed.r, dimmed.g, dimmed.b, dimmed.w)
    
    def DimIfNot(self, not_dim: bool, factor: float = 0.2168) -> 'Color':
        dimmed = super().DimIfNot(not_dim, factor)
        return Color(dimmed.r, dimmed.g, dimmed.b, dimmed.w)