# KeyEvent class for MatrixOS
# Binding of OS/Framework/KeyEvent/KeyEvent.h

from MatrixOS_KeyInfo import KeyInfo

class KeyEvent:
    """Key event structure containing key ID and info"""
    
    # Constructor
    def __init__(self): ...

    # Getters
    def ID(self) -> int: ...
    def KeyInfo(self) -> KeyInfo: ...