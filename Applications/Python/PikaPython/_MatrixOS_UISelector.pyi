# MatrixOS Python Interface - UISelector
# Selector UI component for value selection with lighting modes

from _MatrixOS_UIComponent import UIComponent
from MatrixOS_Color import Color
from MatrixOS_Dimension import Dimension

# Enums
class UISelectorDirection:
    RIGHT_THEN_DOWN: int = 0
    DOWN_THEN_RIGHT: int = 1
    LEFT_THEN_DOWN: int = 2
    DOWN_THEN_LEFT: int = 3
    UP_THEN_RIGHT: int = 4
    RIGHT_THEN_UP: int = 5
    UP_THEN_LEFT: int = 6
    LEFT_THEN_UP: int = 7

class UISelectorLitMode:
    LIT_EQUAL: int = 0
    LIT_LESS_EQUAL_THAN: int = 1
    LIT_GREATER_EQUAL_THAN: int = 2
    LIT_ALWAYS: int = 3

class UISelector(_MatrixOS_UIComponent.UIComponent):
    def __init__(self) -> None: ...

    # Behaviour Functions
    def SetValueFunc(self, getValueFunc: any) -> bool: ...
    def SetColorFunc(self, colorFunc: any) -> bool: ...
    def SetIndividualColorFunc(self, individualColorFunc: any) -> bool: ...
    def SetNameFunc(self, nameFunc: any) -> bool: ...

    # Configuration
    def SetLitMode(self, litMode: int) -> bool: ...
    def SetDimension(self, dimension: Dimension) -> bool: ...
    def SetName(self, name: str) -> bool: ...
    def SetCount(self, count: int) -> bool: ...
    def SetDirection(self, direction: int) -> bool: ...
    def SetColor(self, color: Color) -> bool: ...

    # Callback Functions
    def OnChange(self, changeCallback: any) -> bool: ...