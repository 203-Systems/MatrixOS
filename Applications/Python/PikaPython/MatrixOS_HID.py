import _MatrixOS_HID
import MatrixOS_HID_Keyboard as Keyboard
import MatrixOS_HID_Gamepad as Gamepad
import MatrixOS_HID_RawHID as RawHID
import MatrixOS_HID_Mouse as Mouse
import MatrixOS_HID_Consumer as Consumer
import MatrixOS_HID_System as System
import MatrixOS_HID_Touch as Touch

def Ready() -> bool:
    return _MatrixOS_HID.Ready()