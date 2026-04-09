# MatrixOS Python Interface — InputSnapshot
# Represents the current state of a single input.
#
# Usage pattern:
#   snap = MatrixOS.Input.GetState(input_id)
#   if snap is None:
#       return  # input not found or no state available
#   if snap.InputClass() == MatrixOS.Input.InputClass.KEYPAD:
#       info = snap.Keypad()
#       if info is not None:
#           # use info.State(), info.Force(), etc.

from MatrixOS_InputId import InputId
from MatrixOS_KeypadInfo import KeypadInfo

class InputSnapshot:
    def __init__(self): ...

    # The InputId that this snapshot belongs to
    def Id(self) -> InputId: ...

    # The InputClass enum value (see MatrixOS_InputClass)
    def InputClass(self) -> int: ...

    # Keypad info — returns KeypadInfo if this is a keypad input, else None
    def Keypad(self) -> any: ...

    def __bool__(self) -> bool: ...
