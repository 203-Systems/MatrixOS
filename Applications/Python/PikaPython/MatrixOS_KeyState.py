# KeyState enumeration for MatrixOS
# Binding of keypad state values used by the Input API

class KeyState:
    """Key state enumeration values"""
    IDLE: int = 0
    ACTIVATED: int = 1
    PRESSED: int = 2
    HOLD: int = 3
    AFTERTOUCH: int = 4
    RELEASED: int = 5
    DEBOUNCING: int = 240
    RELEASE_DEBOUNCING: int = 241
    INVALID: int = 255
