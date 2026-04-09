# MatrixOS Python Interface — InputEvent
# Represents a single input event from any input source.
#
# Not every event is a keypad event.  Always check InputClass()
# before calling Keypad():
#
#   event = MatrixOS.Input.GetEvent()
#   if event is not None:
#       if event.InputClass() == InputClass.KEYPAD:
#           info = event.Keypad()
#           if info is not None:
#               # use info.State(), info.Force(), etc.

from MatrixOS_InputId import InputId
from MatrixOS_KeypadInfo import KeypadInfo

class InputEvent:
    def __init__(self): ...

    # The InputId that generated this event.
    def Id(self) -> InputId: ...

    # The InputClass enum value (see MatrixOS_InputClass).
    def InputClass(self) -> int: ...

    # Keypad payload — returns KeypadInfo when InputClass() == KEYPAD,
    # otherwise returns None.
    def Keypad(self) -> any: ...

    def __bool__(self) -> bool: ...
