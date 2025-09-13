# KeyState enumeration for MatrixOS
# Binding of OS/Framework/KeyEvent/KeyEvent.h

class KeyState:
    """Key state enumeration values"""
    IDLE: int = 0
    ACTIVATED: int = 1
    PRESSED: int = 2
    RELEASED: int = 3
    HOLD: int = 4
    AFTERTOUCH: int = 5