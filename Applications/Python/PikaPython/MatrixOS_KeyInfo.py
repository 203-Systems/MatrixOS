# KeyInfo class for MatrixOS
# Binding of OS/Framework/KeyEvent/KeyEvent.h

from MatrixOS_KeyState import KeyState

class KeyInfo:
    """Key information structure containing state and timing data"""
    state: KeyState     # KeyState value
    velocity: int       # Key velocity (0~65535)
    lastEventTime: int  # Timestamp of last event
    hold: bool          # Hold state