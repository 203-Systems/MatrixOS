import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "PikaPython"))

sys.modules["PikaStdLib"] = types.ModuleType("PikaStdLib")


class NativePoint:
    def __init__(self, x=0, y=0):
        self.x_value = x
        self.y_value = y

    def X(self):
        return self.x_value

    def Y(self):
        return self.y_value

    def SetX(self, value):
        self.x_value = value

    def SetY(self, value):
        self.y_value = value


class NativeColor:
    def __init__(self, value=0):
        self.value = value


class NativeInputId:
    def __init__(self, cluster=1, member=7):
        self.cluster = cluster
        self.member = member

    def ClusterId(self):
        return self.cluster

    def MemberId(self):
        return self.member

    def __eq__(self, other):
        return self.cluster == other.cluster and self.member == other.member

    def __bool__(self):
        return self.cluster >= 0

    def FunctionKey():
        return NativeInputId(0, 0)

    def Invalid():
        return NativeInputId(-1, -1)


class NativeInputEvent:
    def Id(self):
        return NativeInputId()

    def InputClass(self):
        return 1

    def ClusterId(self):
        return 1

    def MemberId(self):
        return 7

    def KeyState(self):
        return 2

    def KeyForce(self):
        return 1.0

    def KeyValue(self, index):
        return 1.0

    def KeyHold(self):
        return False

    def KeyActive(self):
        return True

    def Keypad(self):
        return None

    def __bool__(self):
        return True


class NativeInputSnapshot:
    def Id(self):
        return NativeInputId()

    def InputClass(self):
        return 1

    def Keypad(self):
        return None

    def __bool__(self):
        return True


class NativeInputCluster:
    def ClusterId(self):
        return 1

    def Name(self):
        return "Grid"

    def InputClass(self):
        return 1

    def Shape(self):
        return 2

    def RootPoint(self):
        return NativePoint(0, 0)

    def Dimension(self):
        return NativePoint(8, 8)

    def InputCount(self):
        return 64

    def HasRootPoint(self):
        return True

    def HasCoordinates(self):
        return True


class NativeMidiPacket:
    pass


def install_class_stub(module_name, class_name, native_class):
    module = types.ModuleType(module_name)
    setattr(module, class_name, native_class)
    sys.modules[module_name] = module


install_class_stub("_MatrixOS_Point", "Point", NativePoint)
install_class_stub("_MatrixOS_Color", "Color", NativeColor)
install_class_stub("_MatrixOS_Dimension", "Dimension", NativePoint)
install_class_stub("_MatrixOS_InputId", "InputId", NativeInputId)
install_class_stub("_MatrixOS_InputEvent", "InputEvent", NativeInputEvent)
install_class_stub("_MatrixOS_InputSnapshot", "InputSnapshot", NativeInputSnapshot)
install_class_stub("_MatrixOS_InputCluster", "InputCluster", NativeInputCluster)
install_class_stub("_MatrixOS_KeypadInfo", "KeypadInfo", object)
install_class_stub("_MatrixOS_MidiPacket", "MidiPacket", NativeMidiPacket)
sys.modules["_MatrixOS_InputClass"] = types.ModuleType("_MatrixOS_InputClass")

input_native = types.ModuleType("_MatrixOS_Input")
input_native.GetEvent = lambda timeout_ms: NativeInputEvent()
input_native.GetState = lambda input_id: NativeInputSnapshot()
input_native.GetPosition = lambda input_id: NativePoint(2, 3)
input_native.GetInputsAt = lambda point: [NativeInputId()]
input_native.ClearInputBuffer = lambda: None
input_native.FunctionKey = lambda: NativeInputId.FunctionKey()
input_native.GetClusters = lambda: [NativeInputCluster()]
input_native.GetPrimaryGridCluster = lambda: NativeInputCluster()
sys.modules["_MatrixOS_Input"] = input_native

sys_native = types.ModuleType("_MatrixOS_SYS")
sys_native.Reboot = lambda: None
sys_native.Bootloader = lambda: None
sys_native.DelayMs = lambda ms: None
sys_native.TaskYield = lambda: None
sys_native.Millis = lambda: 1000
sys_native.Micros = lambda: 1000000
sys_native.OpenSetting = lambda: None
sys_native.ExecuteAPP = lambda author, app_name, args: None
sys_native.ExecuteAPPByID = lambda app_id, args: None
sys_native.GetVersion = lambda: (4, 0, 1)
sys.modules["_MatrixOS_SYS"] = sys_native

led_native = types.ModuleType("_MatrixOS_LED")
led_native.NextBrightness = lambda: None
led_native.SetBrightness = lambda brightness: None
led_native.SetBrightnessMultiplier = lambda partition, multiplier: True
led_native.SetColor = lambda xy, color, layer: None
led_native.SetColorByID = lambda led_id, color, layer: None
led_native.Fill = lambda color, layer: None
led_native.FillPartition = lambda partition, color, layer: True
led_native.Update = lambda layer: None
led_native.CurrentLayer = lambda: 1
led_native.CreateLayer = lambda crossfade: 2
led_native.CopyLayer = lambda dest, src: None
led_native.DestroyLayer = lambda crossfade: True
led_native.Fade = lambda crossfade: None
led_native.PauseUpdate = lambda pause: None
led_native.GetLEDCount = lambda: 64
led_native.GetPartitionCount = lambda: 0
led_native.GetPartitionIndex = lambda name: -1
led_native.GetPartitionName = lambda index: ""
led_native.GetPartitionStart = lambda index: 0
led_native.GetPartitionSize = lambda index: 0
led_native.GetPartitionType = lambda index: 0
led_native.GetPartitionDefaultMultiplier = lambda index: 0.0
sys.modules["_MatrixOS_LED"] = led_native

midi_native = types.ModuleType("_MatrixOS_MIDI")
midi_native.Get = lambda timeout_ms: None
midi_native.Send = lambda packet, timeout_ms: True
midi_native.SendSysEx = lambda port, length, data, include_meta: True
sys.modules["_MatrixOS_MIDI"] = midi_native

nvs_native = types.ModuleType("_MatrixOS_NVS")
nvs_native.GetSize = lambda key: 0
nvs_native.GetVariable = lambda key: None
nvs_native.SetVariable = lambda key, data, length: True
nvs_native.DeleteVariable = lambda key: True
sys.modules["_MatrixOS_NVS"] = nvs_native

usb_native = types.ModuleType("_MatrixOS_USB")
usb_native.Connected = lambda: True
sys.modules["_MatrixOS_USB"] = usb_native

ui_utility_native = types.ModuleType("_MatrixOS_UIUtility")
ui_utility_native.TextScroll = lambda text, color, speed, loop: None
ui_utility_native.NumberSelector8x8 = lambda value, color, name, lower, upper: value
ui_utility_native.ColorPicker = lambda: None
sys.modules["_MatrixOS_UIUtility"] = ui_utility_native

for name in (
    "_MatrixOS_HID",
    "_MatrixOS_HID_Keyboard",
    "_MatrixOS_HID_Gamepad",
    "_MatrixOS_HID_RawHID",
    "_MatrixOS_HID_Mouse",
    "_MatrixOS_HID_Consumer",
    "_MatrixOS_HID_System",
    "_MatrixOS_HID_Touch",
    "_MatrixOS_ColorEffects",
    "_MatrixOS_Utils",
    "_MatrixOS_UI",
    "_MatrixOS_UIComponent",
    "_MatrixOS_UIButton",
    "_MatrixOS_UISelector",
    "_MatrixOS_UI4pxNumber",
):
    module = types.ModuleType(name)
    sys.modules[name] = module

sys.modules["_MatrixOS_HID"].Ready = lambda: True
sys.modules["_MatrixOS_HID_Keyboard"].Tap = lambda keycode, length_ms: True
sys.modules["_MatrixOS_HID_Keyboard"].Press = lambda keycode: True
sys.modules["_MatrixOS_HID_Keyboard"].Release = lambda keycode: True
sys.modules["_MatrixOS_HID_Keyboard"].ReleaseAll = lambda: None
sys.modules["_MatrixOS_HID_RawHID"].Get = lambda timeout_ms: b""
sys.modules["_MatrixOS_HID_RawHID"].Send = lambda report, length: True
sys.modules["_MatrixOS_ColorEffects"].Rainbow = lambda period, offset: NativeColor()
sys.modules["_MatrixOS_Utils"].StringHash = lambda text: len(text)
sys.modules["_MatrixOS_UI"].UI = object
sys.modules["_MatrixOS_UIComponent"].UIComponent = object
sys.modules["_MatrixOS_UIButton"].UIButton = object
sys.modules["_MatrixOS_UISelector"].UISelector = object
sys.modules["_MatrixOS_UISelector"].UISelectorDirection = object
sys.modules["_MatrixOS_UISelector"].UISelectorLitMode = object
sys.modules["_MatrixOS_UI4pxNumber"].UI4pxNumber = object

for name in ("Gamepad", "Mouse", "Consumer", "System", "Touch"):
    module = sys.modules["_MatrixOS_HID_" + name]
    module.Click = lambda *args: None
    module.Tap = lambda *args: None
    module.Press = lambda *args: None
    module.Release = lambda *args: None
    module.ReleaseAll = lambda: None
    module.Button = lambda *args: None
    module.Buttons = lambda *args: None
    module.XAxis = lambda *args: None
    module.YAxis = lambda *args: None
    module.ZAxis = lambda *args: None
    module.RXAxis = lambda *args: None
    module.RYAxis = lambda *args: None
    module.RZAxis = lambda *args: None
    module.DPad = lambda *args: None
    module.Write = lambda *args: None
    module.MoveTo = lambda *args: None
    module.Move = lambda *args: None


import MatrixOS


def test_matrixos_top_level_facade_is_pythonic_only():
    assert MatrixOS.Input.FUNCTION_KEY == MatrixOS.Input.function_key()
    assert MatrixOS.Input.get_position(NativeInputId()).x() == 2
    assert MatrixOS.SYS.millis() == 1000
    assert MatrixOS.LED.fill_partition("Grid", NativeColor())
    assert MatrixOS.HID.ready()
    assert MatrixOS.USB.connected()
    assert MatrixOS.UI.Button is not None
    assert MatrixOS.Timer is not None
    assert MatrixOS.ColorEffects.rainbow() is not None

    assert not hasattr(MatrixOS.Input, "GetPosition")
    assert not hasattr(MatrixOS.Input, "FunctionKey")
    assert not hasattr(MatrixOS.SYS, "DelayMs")
    assert not hasattr(MatrixOS.SYS, "TaskYield")
    assert not hasattr(MatrixOS.LED, "FillPartition")
    assert not hasattr(MatrixOS.MIDI, "Send")
    assert not hasattr(MatrixOS.UIUtility, "TextScroll")


if __name__ == "__main__":
    test_matrixos_top_level_facade_is_pythonic_only()
