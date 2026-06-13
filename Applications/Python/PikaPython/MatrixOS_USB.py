import _MatrixOS_USB

def Connected() -> bool:
    return _MatrixOS_USB.Connected()

def connected() -> bool:
    return Connected()
