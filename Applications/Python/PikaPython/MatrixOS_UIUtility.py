import _MatrixOS_UIUtility
from MatrixOS_Color import Color, _wrap


INT_MIN = -2147483648
INT_MAX = 2147483647


def text_scroll(text: str, color: Color, speed: int = 10, loop: bool = False) -> None:
    if hasattr(color, "native"):
        color = color.native
    _MatrixOS_UIUtility.TextScroll(text, color, speed, loop)


def number_selector_8x8(value: int, color: Color, name: str, lower_limit: int = INT_MIN, upper_limit: int = INT_MAX) -> int:
    if hasattr(color, "native"):
        color = color.native
    return _MatrixOS_UIUtility.NumberSelector8x8(value, color, name, lower_limit, upper_limit)


def color_picker() -> any:
    color = _MatrixOS_UIUtility.ColorPicker()
    if color is None:
        return None
    return _wrap(color)
