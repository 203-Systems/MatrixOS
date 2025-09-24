# KeyState enumeration for MatrixOS
# Binding of OS/Framework/KeyEvent/KeyInfo.h

class KeyState:
    """Key state enumeration values"""
    INVALID: int = 0
    IDLE: int = 1
    ACTIVATED: int = 2
    PRESSED: int = 3
    RELEASED: int = 4
    HOLD: int = 5
    AFTERTOUCH: int = 6