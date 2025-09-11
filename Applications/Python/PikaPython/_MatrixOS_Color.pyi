# Color class for MatrixOS
# Binding of OS/Framework/Types/Color.h

class Color:
    """RGB color representation"""
    def __init__(self, r: int, g: int, b: int, w: int): ...

    # Color properties
    r: int
    g: int
    b: int
    w: int
    
    # Color manipulation methods
    def RGB(self) -> int: ...
    def Scale(self, factor: float) -> 'Color': ...
    def Dim(self, factor: float) -> 'Color': ...
    def DimIfNot(self, not_dim: bool, factor: float) -> 'Color': ...
    
    # Color operators
    def __eq__(self, other: 'Color') -> bool: ...
    def __ne__(self, other: 'Color') -> bool: ...
    
  