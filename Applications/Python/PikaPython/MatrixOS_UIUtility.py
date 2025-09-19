import _MatrixOS_UIUtility as _MatrixOS_UIUtility
from MatrixOS_Color import Color

# Constants for default values
INT_MIN = -2147483648  # INT32_MIN
INT_MAX = 2147483647   # INT32_MAX

def TextScroll(text: str, color: Color, speed: int = 10, loop: bool = False) -> None:
    """Scroll text across the 8x8 LED matrix

    Args:
        text: Text string to display
        color: Color for the text
        speed: Scroll speed (default: 10)
        loop: Whether to loop the text (default: False)
    """
    _MatrixOS_UIUtility.TextScroll(text, color, speed, loop)

def NumberSelector8x8(value: int, color: Color, name: str, lower_limit: int = INT_MIN, upper_limit: int = INT_MAX) -> int:
    """Interactive number selector on 8x8 grid

    Args:
        value: Initial value
        color: UI color
        name: Display name for the selector
        lower_limit: Minimum allowed value (default: INT_MIN)
        upper_limit: Maximum allowed value (default: INT_MAX)

    Returns:
        Selected number value
    """
    return _MatrixOS_UIUtility.NumberSelector8x8(value, color, name, lower_limit, upper_limit)

def ColorPicker() -> any:
    """Interactive color picker interface

    Returns:
        Selected color or None
    """
    return _MatrixOS_UIUtility.ColorPicker()