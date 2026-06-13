import _MatrixOS_Color

class Color(_MatrixOS_Color.Color):
    def r(self) -> int:
        return self.R()

    def g(self) -> int:
        return self.G()

    def b(self) -> int:
        return self.B()

    def w(self) -> int:
        return self.W()

    def rgb(self) -> int:
        return self.RGB()

    def wrgb(self) -> int:
        return self.WRGB()

    def set_r(self, r: int) -> None:
        self.SetR(r)

    def set_g(self, g: int) -> None:
        self.SetG(g)

    def set_b(self, b: int) -> None:
        self.SetB(b)

    def set_w(self, w: int) -> None:
        self.SetW(w)

    def scale(self, factor: float):
        return self.Scale(factor)

    def Dim(self, factor: float = 0.2168):
        return super().Dim(factor)

    def dim(self, factor: float = 0.2168):
        return self.Dim(factor)
    
    def DimIfNot(self, not_dim: bool, factor: float = 0.2168):
        return super().DimIfNot(not_dim, factor)

    def dim_if_not(self, not_dim: bool, factor: float = 0.2168):
        return self.DimIfNot(not_dim, factor)

def rgb(r: int, g: int, b: int):
    return Color(r, g, b)

def rgbw(r: int, g: int, b: int, w: int):
    return Color(r, g, b, w)

def hex(wrgb: int):
    return Color(wrgb)
