import _MatrixOS_Color


def _wrap(native):
    color = Color(0)
    color.native = native
    return color


def _native(color):
    if hasattr(color, "native"):
        return color.native
    return color


class Color:
    def __init__(self, *args):
        self.native = _MatrixOS_Color.Color(*args)

    def r(self) -> int:
        return self.native.R()

    def g(self) -> int:
        return self.native.G()

    def b(self) -> int:
        return self.native.B()

    def w(self) -> int:
        return self.native.W()

    def rgb(self) -> int:
        return self.native.RGB()

    def wrgb(self) -> int:
        return self.native.WRGB()

    def set_r(self, r: int) -> None:
        self.native.SetR(r)

    def set_g(self, g: int) -> None:
        self.native.SetG(g)

    def set_b(self, b: int) -> None:
        self.native.SetB(b)

    def set_w(self, w: int) -> None:
        self.native.SetW(w)

    def scale(self, factor: float):
        return _wrap(self.native.Scale(factor))

    def dim(self, factor: float = 0.2168):
        return _wrap(self.native.Dim(factor))

    def dim_if_not(self, not_dim: bool, factor: float = 0.2168):
        if not_dim:
            return self
        return self.dim(factor)

    def raw(self):
        return self.native


def rgb(r: int, g: int, b: int):
    return Color(r, g, b)


def rgbw(r: int, g: int, b: int, w: int):
    return Color(r, g, b, w)


def hex(wrgb: int):
    return Color(wrgb)
