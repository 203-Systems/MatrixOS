# MatrixOS Python Interface - UIComponent
# Base class for UI components

from MatrixOS_Point import Point
from MatrixOS_Dimension import Dimension
from MatrixOS_KeyInfo import KeyInfo

class UIComponent:
    def __init__(self) -> None: ...

    def SetEnabled(self, enabled: bool) -> None: ...
    def ShouldEnable(self, enable_func: any) -> None: ...