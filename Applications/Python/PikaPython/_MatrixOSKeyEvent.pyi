# KeyEvent definitions for MatrixOS
# Binding of OS/Framework/KeyEvent/KeyEvent.h

from _MatrixOSPoint import Point

# KeyState enumeration
class KeyState:
    IDLE: int = 0
    ACTIVATED: int = 1
    PRESSED: int = 2
    RELEASED: int = 3
    HOLD: int = 4
    AFTERTOUCH: int = 5

class KeyInfo:
    """Key information structure containing state and timing data"""
    state: KeyState     # KeyState value
    velocity: int       # Key velocity (0~65535)
    lastEventTime: int  # Timestamp of last event
    hold: bool          # Hold state 

class KeyEvent:
    """Key event structure containing key ID and info"""
    id: int            # Key identifier
    info: KeyInfo      # Key information