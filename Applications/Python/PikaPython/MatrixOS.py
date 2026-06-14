import PikaStdLib

import MatrixOS_Framework as _Framework
import MatrixOS_HID as _HID
import MatrixOS_HID_Consumer as _HID_Consumer
import MatrixOS_HID_Gamepad as _HID_Gamepad
import MatrixOS_HID_Keyboard as _HID_Keyboard
import MatrixOS_HID_Mouse as _HID_Mouse
import MatrixOS_HID_RawHID as _HID_RawHID
import MatrixOS_HID_System as _HID_System
import MatrixOS_HID_Touch as _HID_Touch
import MatrixOS_Input as _Input
import MatrixOS_LED as _LED
import MatrixOS_MIDI as _MIDI
import MatrixOS_NVS as _NVS
import MatrixOS_SYS as _SYS
import MatrixOS_Timer as _Timer
import MatrixOS_UI as _UI
import MatrixOS_UIUtility as _UIUtility
import MatrixOS_USB as _USB


class _SystemAPI:
    def reboot(self) -> None:
        _SYS.reboot()

    def bootloader(self) -> None:
        _SYS.bootloader()

    def sleep_ms(self, ms: int) -> None:
        _SYS.sleep_ms(ms)

    def delay_ms(self, ms: int) -> None:
        _SYS.delay_ms(ms)

    def task_yield(self) -> None:
        _SYS.task_yield()

    def yield_(self) -> None:
        _SYS.task_yield()

    def millis(self) -> int:
        return _SYS.millis()

    def micros(self) -> int:
        return _SYS.micros()

    def open_settings(self) -> None:
        _SYS.open_settings()

    def launch_app(self, author: str, app_name: str, args: list = None) -> None:
        _SYS.launch_app(author, app_name, args)

    def launch_app_by_id(self, app_id: int, args: list = None) -> None:
        _SYS.launch_app_by_id(app_id, args)

    def get_version(self) -> tuple:
        return _SYS.get_version()

    def version(self):
        return _SYS.version()


class _LedAPI:
    CROSSFADE_DURATION = _LED.CROSSFADE_DURATION
    CURRENT_LAYER = _LED.CURRENT_LAYER
    LED_TYPE_MONO_1B = _LED.LED_TYPE_MONO_1B
    LED_TYPE_MONO_8B = _LED.LED_TYPE_MONO_8B
    LED_TYPE_RGB_24B = _LED.LED_TYPE_RGB_24B
    LED_TYPE_RGBW_32B_6K5 = _LED.LED_TYPE_RGBW_32B_6K5
    LEDPartition = _LED.LEDPartition

    def next_brightness(self) -> None:
        _LED.next_brightness()

    def set_brightness(self, brightness: int) -> None:
        _LED.set_brightness(brightness)

    def set_brightness_multiplier(self, partition_name: str, multiplier: float) -> bool:
        return _LED.set_brightness_multiplier(partition_name, multiplier)

    def set_color(self, xy, color, layer: int = _LED.CURRENT_LAYER) -> None:
        _LED.set_color(xy, color, layer)

    def set_color_xy(self, x: int, y: int, color, layer: int = _LED.CURRENT_LAYER) -> None:
        _LED.set_color_xy(x, y, color, layer)

    def set_color_by_id(self, led_id: int, color, layer: int = _LED.CURRENT_LAYER) -> None:
        _LED.set_color_by_id(led_id, color, layer)

    def set_pixel(self, led_id: int, color, layer: int = _LED.CURRENT_LAYER) -> None:
        _LED.set_pixel(led_id, color, layer)

    def fill(self, color, layer: int = _LED.CURRENT_LAYER) -> None:
        _LED.fill(color, layer)

    def clear(self, layer: int = _LED.CURRENT_LAYER) -> None:
        _LED.clear(layer)

    def fill_partition(self, partition: str, color, layer: int = _LED.CURRENT_LAYER) -> bool:
        return _LED.fill_partition(partition, color, layer)

    def update(self, layer: int = _LED.CURRENT_LAYER) -> None:
        _LED.update(layer)

    def show(self, layer: int = _LED.CURRENT_LAYER) -> None:
        _LED.show(layer)

    def current_layer(self) -> int:
        return _LED.current_layer()

    def create_layer(self, crossfade: int = _LED.CROSSFADE_DURATION) -> int:
        return _LED.create_layer(crossfade)

    def copy_layer(self, dest: int, src: int) -> None:
        _LED.copy_layer(dest, src)

    def destroy_layer(self, crossfade: int = _LED.CROSSFADE_DURATION) -> bool:
        return _LED.destroy_layer(crossfade)

    def fade(self, crossfade: int = _LED.CROSSFADE_DURATION) -> None:
        _LED.fade(crossfade)

    def pause_update(self, pause: bool = True) -> None:
        _LED.pause_update(pause)

    def get_led_count(self) -> int:
        return _LED.get_led_count()

    def count(self) -> int:
        return _LED.count()

    def partition_count(self) -> int:
        return _LED.partition_count()

    def get_partition(self, index: int):
        return _LED.get_partition(index)

    def get_partition_by_name(self, name: str):
        return _LED.get_partition_by_name(name)

    def get_partitions(self) -> list:
        return _LED.get_partitions()

    def partitions(self) -> list:
        return _LED.partitions()


class _InputAPI:
    def __init__(self):
        self.FUNCTION_KEY = _Input.function_key()

    def get_event(self, timeout_ms: int = 0):
        return _Input.get_event(timeout_ms)

    def get_state(self, input_id):
        return _Input.get_state(input_id)

    def get_position(self, input_id):
        return _Input.get_position(input_id)

    def try_get_point(self, input_id):
        return _Input.try_get_point(input_id)

    def get_inputs_at(self, point) -> list:
        return _Input.get_inputs_at(point)

    def clear_input_buffer(self) -> None:
        _Input.clear_input_buffer()

    def function_key(self):
        return self.FUNCTION_KEY

    def get_clusters(self) -> list:
        return _Input.get_clusters()

    def clusters(self) -> list:
        return _Input.clusters()

    def get_cluster(self, target):
        return _Input.get_cluster(target)

    def get_cluster_name(self, cluster_id: int) -> str:
        return _Input.get_cluster_name(cluster_id)

    def get_keypad_clusters(self) -> list:
        return _Input.get_keypad_clusters()

    def keypad_clusters(self) -> list:
        return _Input.keypad_clusters()

    def get_keypad_cluster(self, name: str = ""):
        return _Input.get_keypad_cluster(name)

    def keypad_cluster(self, name: str = ""):
        return _Input.keypad_cluster(name)

    def get_primary_grid_cluster(self):
        return _Input.get_primary_grid_cluster()

    def primary_grid(self):
        return _Input.primary_grid()


class _FrameworkAPI:
    Timer = _Timer.Timer
    Point = _Framework.Point
    Direction = _Framework.Direction
    Dimension = _Framework.Dimension
    Color = _Framework.Color
    KeypadInfo = _Framework.KeypadInfo
    InputEvent = _Framework.InputEvent
    InputSnapshot = _Framework.InputSnapshot
    InputCluster = _Framework.InputCluster
    KeyState = _Framework.KeyState
    KeyboardKeycode = _Framework.KeyboardKeycode
    MouseKeycode = _Framework.MouseKeycode
    GamepadDPadDirection = _Framework.GamepadDPadDirection
    ConsumerKeycode = _Framework.ConsumerKeycode
    SystemKeycode = _Framework.SystemKeycode
    MidiPortID = _Framework.MidiPortID
    MidiStatus = _Framework.MidiStatus
    MidiPacket = _Framework.MidiPacket
    ColorEffects = _Framework.ColorEffects


class _UiAPI:
    UI = _UI.UI
    Component = _UI.Component
    Button = _UI.Button
    Selector = _UI.Selector
    Number = _UI.Number
    KeyEvent = _UI.KeyEvent


class _MidiAPI:
    note_on = _MIDI.note_on
    note_off = _MIDI.note_off
    aftertouch = _MIDI.aftertouch
    control_change = _MIDI.control_change
    cc = _MIDI.cc
    program_change = _MIDI.program_change
    channel_pressure = _MIDI.channel_pressure
    pitch_bend = _MIDI.pitch_bend
    clock = _MIDI.clock
    start = _MIDI.start
    continue_ = _MIDI.continue_
    stop = _MIDI.stop
    active_sense = _MIDI.active_sense
    reset = _MIDI.reset
    song_position = _MIDI.song_position
    song_select = _MIDI.song_select
    tune_request = _MIDI.tune_request
    port = _MIDI.port
    status = _MIDI.status
    channel = _MIDI.channel
    note = _MIDI.note
    controller = _MIDI.controller
    velocity = _MIDI.velocity
    value = _MIDI.value
    set_status = _MIDI.set_status
    set_channel = _MIDI.set_channel
    set_note = _MIDI.set_note
    set_controller = _MIDI.set_controller
    set_velocity = _MIDI.set_velocity
    set_value = _MIDI.set_value
    length = _MIDI.length
    is_sysex = _MIDI.is_sysex
    is_sysex_start = _MIDI.is_sysex_start

    def get(self, timeout_ms: int = 0):
        return _MIDI.get(timeout_ms)

    def send(self, packet, timeout_ms: int = 0) -> bool:
        return _MIDI.send(packet, timeout_ms)

    def send_sysex(self, port: int, data: bytes, include_meta: bool = False) -> bool:
        return _MIDI.send_sysex(port, data, include_meta)


class _NvsAPI:
    get_u8 = _NVS.get_u8
    set_u8 = _NVS.set_u8
    get_u16 = _NVS.get_u16
    set_u16 = _NVS.set_u16
    get_u32 = _NVS.get_u32
    set_u32 = _NVS.set_u32
    get_str = _NVS.get_str
    set_str = _NVS.set_str

    def get_size(self, key: int) -> int:
        return _NVS.get_size(key)

    def get(self, key: int) -> bytes:
        return _NVS.get(key)

    def set(self, key: int, data: bytes) -> bool:
        return _NVS.set(key, data)

    def delete(self, key: int) -> bool:
        return _NVS.delete(key)


class _KeyboardAPI:
    tap = _HID_Keyboard.tap
    press = _HID_Keyboard.press
    release = _HID_Keyboard.release
    release_all = _HID_Keyboard.release_all


class _GamepadAPI:
    tap = _HID_Gamepad.tap
    press = _HID_Gamepad.press
    release = _HID_Gamepad.release
    release_all = _HID_Gamepad.release_all
    button = _HID_Gamepad.button
    buttons = _HID_Gamepad.buttons
    x_axis = _HID_Gamepad.x_axis
    y_axis = _HID_Gamepad.y_axis
    z_axis = _HID_Gamepad.z_axis
    rx_axis = _HID_Gamepad.rx_axis
    ry_axis = _HID_Gamepad.ry_axis
    rz_axis = _HID_Gamepad.rz_axis
    dpad = _HID_Gamepad.dpad


class _MouseAPI:
    click = _HID_Mouse.click
    press = _HID_Mouse.press
    release = _HID_Mouse.release
    release_all = _HID_Mouse.release_all
    move_to = _HID_Mouse.move_to
    move = _HID_Mouse.move


class _RawHidAPI:
    get = _HID_RawHID.get
    send = _HID_RawHID.send


class _ConsumerAPI:
    write = _HID_Consumer.write
    press = _HID_Consumer.press
    release = _HID_Consumer.release
    release_all = _HID_Consumer.release_all


class _SystemHidAPI:
    write = _HID_System.write
    press = _HID_System.press
    release = _HID_System.release
    release_all = _HID_System.release_all


class _TouchAPI:
    click = _HID_Touch.click
    press = _HID_Touch.press
    release = _HID_Touch.release
    release_all = _HID_Touch.release_all
    move_to = _HID_Touch.move_to
    move = _HID_Touch.move


class _HidAPI:
    Keyboard = _KeyboardAPI()
    Gamepad = _GamepadAPI()
    RawHID = _RawHidAPI()
    Mouse = _MouseAPI()
    Consumer = _ConsumerAPI()
    System = _SystemHidAPI()
    Touch = _TouchAPI()

    def ready(self) -> bool:
        return _HID.ready()


class _UsbAPI:
    def connected(self) -> bool:
        return _USB.connected()


class _UiUtilityAPI:
    def text_scroll(self, text: str, color, speed: int = 10, loop: bool = False) -> None:
        _UIUtility.text_scroll(text, color, speed, loop)

    def number_selector_8x8(self, value: int, color, name: str, lower_limit: int = _UIUtility.INT_MIN,
                            upper_limit: int = _UIUtility.INT_MAX) -> int:
        return _UIUtility.number_selector_8x8(value, color, name, lower_limit, upper_limit)

    def color_picker(self):
        return _UIUtility.color_picker()


SYS = _SystemAPI()
LED = _LedAPI()
Input = _InputAPI()
Framework = _FrameworkAPI()
UI = _UiAPI()
Timer = _Timer.Timer
Color = _Framework.Color
Point = _Framework.Point
Dimension = _Framework.Dimension
KeyState = _Framework.KeyState
ColorEffects = _Framework.ColorEffects
MIDI = _MidiAPI()
NVS = _NvsAPI()
HID = _HidAPI()
USB = _UsbAPI()
UIUtility = _UiUtilityAPI()
