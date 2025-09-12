import _MatrixOS_USB
import MatrixOS_USB_CDC as CDC

def Connected() -> bool:
    return _MatrixOS_USB.Connected()