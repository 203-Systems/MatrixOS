# Direction enum for MatrixOS
# Binding of OS/Framework/Geometry/Direction.h

class Direction:
    """Direction enumeration with multiple naming conventions"""
    
    # Primary directions (0°, 90°, 180°, 270°)
    UP: int = 0
    TOP: int = 0
    NORTH: int = 0
    
    RIGHT: int = 90
    EAST: int = 90
    
    DOWN: int = 180
    BOTTOM: int = 180
    SOUTH: int = 180
    
    LEFT: int = 270
    WEST: int = 270
    
    # Diagonal directions (45°, 135°, 225°, 315°)
    TOPRIGHT: int = 45
    UPPERRIGHT: int = 45
    NORTHEAST: int = 45
    
    DOWNRIGHT: int = 135
    BOTTOMRIGHT: int = 135
    SOUTHEAST: int = 135
    
    DOWNLEFT: int = 225
    BOTTOMLEFT: int = 225
    SOUTHLEFT: int = 225
    
    UPPERLEFT: int = 315
    TOPLEFT: int = 315
    NORTHWEST: int = 315