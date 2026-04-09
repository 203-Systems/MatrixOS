# MatrixOS Python Interface — InputEvent
# Represents a single input event from any input source.

from MatrixOS_InputId import InputId
from MatrixOS_KeypadInfo import KeypadInfo

class InputEvent:
    def __init__(self): ...

    # The InputId that generated this event
    def Id(self) -> InputId: ...

    # The InputClass enum value (see MatrixOS_InputClass)
    def InputClass(self) -> int: ...

    # Keypad info — returns KeypadInfo if this is a keypad event, else None
    def Keypad(self) -> any: ...

    def __bool__(self) -> bool: ...
