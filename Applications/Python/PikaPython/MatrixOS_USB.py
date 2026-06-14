import _MatrixOS_USB


def connected() -> bool:
    return _MatrixOS_USB.Connected()
