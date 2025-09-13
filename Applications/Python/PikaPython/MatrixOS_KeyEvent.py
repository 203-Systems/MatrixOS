# KeyEvent class for MatrixOS
# Binding of OS/Framework/KeyEvent/KeyEvent.h

from MatrixOS_KeyInfo import KeyInfo

class KeyEvent:
    """Key event structure containing key ID and info"""
    id: int            # Key identifier
    info: KeyInfo      # Key information