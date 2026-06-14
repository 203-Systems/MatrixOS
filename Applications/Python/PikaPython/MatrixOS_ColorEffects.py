import _MatrixOS_ColorEffects
from MatrixOS_Color import Color, _native, _wrap


def rainbow(period: int = 1000, offset: int = 0) -> Color:
    return _wrap(_MatrixOS_ColorEffects.Rainbow(period, offset))


def breath(period: int = 1000, offset: int = 0) -> int:
    return _MatrixOS_ColorEffects.Breath(period, offset)


def breath_low_bound(low_bound: int = 64, period: int = 1000, offset: int = 0) -> int:
    return _MatrixOS_ColorEffects.BreathLowBound(low_bound, period, offset)


def strobe(period: int = 1000, offset: int = 0) -> int:
    return _MatrixOS_ColorEffects.Strobe(period, offset)


def saw(period: int = 1000, offset: int = 0) -> int:
    return _MatrixOS_ColorEffects.Saw(period, offset)


def triangle(period: int = 1000, offset: int = 0) -> int:
    return _MatrixOS_ColorEffects.Triangle(period, offset)


def color_breath(color: Color, period: int = 1000, offset: int = 0) -> Color:
    return _wrap(_MatrixOS_ColorEffects.ColorBreath(_native(color), period, offset))


def color_breath_low_bound(color: Color, low_bound: int = 64, period: int = 1000, offset: int = 0) -> Color:
    return _wrap(_MatrixOS_ColorEffects.ColorBreathLowBound(_native(color), low_bound, period, offset))


def color_strobe(color: Color, period: int = 1000, offset: int = 0) -> Color:
    return _wrap(_MatrixOS_ColorEffects.ColorStrobe(_native(color), period, offset))


def color_saw(color: Color, period: int = 1000, offset: int = 0) -> Color:
    return _wrap(_MatrixOS_ColorEffects.ColorSaw(_native(color), period, offset))


def color_triangle(color: Color, period: int = 1000, offset: int = 0) -> Color:
    return _wrap(_MatrixOS_ColorEffects.ColorTriangle(_native(color), period, offset))


class ColorEffects:
    def rainbow(period: int = 1000, offset: int = 0) -> Color:
        return rainbow(period, offset)

    def breath(period: int = 1000, offset: int = 0) -> int:
        return breath(period, offset)

    def breath_low_bound(low_bound: int = 64, period: int = 1000, offset: int = 0) -> int:
        return breath_low_bound(low_bound, period, offset)

    def strobe(period: int = 1000, offset: int = 0) -> int:
        return strobe(period, offset)

    def saw(period: int = 1000, offset: int = 0) -> int:
        return saw(period, offset)

    def triangle(period: int = 1000, offset: int = 0) -> int:
        return triangle(period, offset)

    def color_breath(color: Color, period: int = 1000, offset: int = 0) -> Color:
        return color_breath(color, period, offset)

    def color_breath_low_bound(color: Color, low_bound: int = 64, period: int = 1000, offset: int = 0) -> Color:
        return color_breath_low_bound(color, low_bound, period, offset)

    def color_strobe(color: Color, period: int = 1000, offset: int = 0) -> Color:
        return color_strobe(color, period, offset)

    def color_saw(color: Color, period: int = 1000, offset: int = 0) -> Color:
        return color_saw(color, period, offset)

    def color_triangle(color: Color, period: int = 1000, offset: int = 0) -> Color:
        return color_triangle(color, period, offset)
