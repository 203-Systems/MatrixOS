# HID Enums for MatrixOS
# Binding of OS/HID/HIDSpecs.h

class KeyboardKeycode:
    """Keyboard key codes for HID"""
    # Basic keys
    KEY_RESERVED = 0
    KEY_ERROR_ROLLOVER = 1
    KEY_POST_FAIL = 2
    KEY_ERROR_UNDEFINED = 3
    
    # Letters
    KEY_A = 4
    KEY_B = 5
    KEY_C = 6
    KEY_D = 7
    KEY_E = 8
    KEY_F = 9
    KEY_G = 10
    KEY_H = 11
    KEY_I = 12
    KEY_J = 13
    KEY_K = 14
    KEY_L = 15
    KEY_M = 16
    KEY_N = 17
    KEY_O = 18
    KEY_P = 19
    KEY_Q = 20
    KEY_R = 21
    KEY_S = 22
    KEY_T = 23
    KEY_U = 24
    KEY_V = 25
    KEY_W = 26
    KEY_X = 27
    KEY_Y = 28
    KEY_Z = 29
    
    # Numbers
    KEY_1 = 30
    KEY_2 = 31
    KEY_3 = 32
    KEY_4 = 33
    KEY_5 = 34
    KEY_6 = 35
    KEY_7 = 36
    KEY_8 = 37
    KEY_9 = 38
    KEY_0 = 39
    
    # Control keys
    KEY_ENTER = 40
    KEY_RETURN = 40  # Alias
    KEY_ESC = 41
    KEY_BACKSPACE = 42
    KEY_TAB = 43
    KEY_SPACE = 44
    KEY_MINUS = 45
    KEY_EQUAL = 46
    KEY_LEFT_BRACE = 47
    KEY_RIGHT_BRACE = 48
    KEY_BACKSLASH = 49
    KEY_SEMICOLON = 51
    KEY_QUOTE = 52
    KEY_TILDE = 53
    KEY_COMMA = 54
    KEY_PERIOD = 55
    KEY_SLASH = 56
    
    # Function keys
    KEY_CAPS_LOCK = 0x39
    KEY_F1 = 0x3A
    KEY_F2 = 0x3B
    KEY_F3 = 0x3C
    KEY_F4 = 0x3D
    KEY_F5 = 0x3E
    KEY_F6 = 0x3F
    KEY_F7 = 0x40
    KEY_F8 = 0x41
    KEY_F9 = 0x42
    KEY_F10 = 0x43
    KEY_F11 = 0x44
    KEY_F12 = 0x45
    
    # Navigation keys
    KEY_PRINT = 0x46
    KEY_PRINTSCREEN = 0x46  # Alias
    KEY_SCROLL_LOCK = 0x47
    KEY_PAUSE = 0x48
    KEY_INSERT = 0x49
    KEY_HOME = 0x4A
    KEY_PAGE_UP = 0x4B
    KEY_DELETE = 0x4C
    KEY_END = 0x4D
    KEY_PAGE_DOWN = 0x4E
    KEY_RIGHT_ARROW = 0x4F
    KEY_LEFT_ARROW = 0x50
    KEY_DOWN_ARROW = 0x51
    KEY_UP_ARROW = 0x52
    KEY_RIGHT = 0x4F  # Alias
    KEY_LEFT = 0x50   # Alias
    KEY_DOWN = 0x51   # Alias
    KEY_UP = 0x52     # Alias
    
    # Keypad
    KEY_NUM_LOCK = 0x53
    KEYPAD_DIVIDE = 0x54
    KEYPAD_MULTIPLY = 0x55
    KEYPAD_SUBTRACT = 0x56
    KEYPAD_ADD = 0x57
    KEYPAD_ENTER = 0x58
    KEYPAD_1 = 0x59
    KEYPAD_2 = 0x5A
    KEYPAD_3 = 0x5B
    KEYPAD_4 = 0x5C
    KEYPAD_5 = 0x5D
    KEYPAD_6 = 0x5E
    KEYPAD_7 = 0x5F
    KEYPAD_8 = 0x60
    KEYPAD_9 = 0x61
    KEYPAD_0 = 0x62
    KEYPAD_DOT = 0x63
    
    # Modifier keys
    KEY_LEFT_CTRL = 0xE0
    KEY_LEFT_SHIFT = 0xE1
    KEY_LEFT_ALT = 0xE2
    KEY_LEFT_GUI = 0xE3
    KEY_LEFT_WINDOWS = 0xE3  # Alias
    KEY_RIGHT_CTRL = 0xE4
    KEY_RIGHT_SHIFT = 0xE5
    KEY_RIGHT_ALT = 0xE6
    KEY_RIGHT_GUI = 0xE7
    KEY_RIGHT_WINDOWS = 0xE7  # Alias

class MouseKeycode:
    """Mouse button codes for HID"""
    MOUSE_LEFT = 0x01
    MOUSE_RIGHT = 0x02
    MOUSE_MIDDLE = 0x04
    MOUSE_PREV = 0x08
    MOUSE_NEXT = 0x10
    MOUSE_ALL = 0x1F

class GamepadDPadDirection:
    """Gamepad D-Pad direction values"""
    GAMEPAD_DPAD_CENTERED = 0
    GAMEPAD_DPAD_UP = 1
    GAMEPAD_DPAD_UP_RIGHT = 2
    GAMEPAD_DPAD_RIGHT = 3
    GAMEPAD_DPAD_DOWN_RIGHT = 4
    GAMEPAD_DPAD_DOWN = 5
    GAMEPAD_DPAD_DOWN_LEFT = 6
    GAMEPAD_DPAD_LEFT = 7
    GAMEPAD_DPAD_UP_LEFT = 8

class ConsumerKeycode:
    """Consumer control key codes for HID (media keys, etc.)"""
    # Power controls
    CONSUMER_POWER = 0x30
    CONSUMER_SLEEP = 0x32
    
    # Media controls
    MEDIA_RECORD = 0xB2
    MEDIA_FAST_FORWARD = 0xB3
    MEDIA_REWIND = 0xB4
    MEDIA_NEXT = 0xB5
    MEDIA_PREVIOUS = 0xB6
    MEDIA_PREV = 0xB6  # Alias
    MEDIA_STOP = 0xB7
    MEDIA_PLAY_PAUSE = 0xCD
    MEDIA_PAUSE = 0xB0
    
    # Volume controls
    MEDIA_VOLUME_MUTE = 0xE2
    MEDIA_VOL_MUTE = 0xE2  # Alias
    MEDIA_VOLUME_UP = 0xE9
    MEDIA_VOL_UP = 0xE9    # Alias
    MEDIA_VOLUME_DOWN = 0xEA
    MEDIA_VOL_DOWN = 0xEA  # Alias
    
    # Display controls
    CONSUMER_BRIGHTNESS_UP = 0x006F
    CONSUMER_BRIGHTNESS_DOWN = 0x0070
    CONSUMER_SCREENSAVER = 0x19E
    
    # Application launches
    CONSUMER_EMAIL_READER = 0x18A
    CONSUMER_CALCULATOR = 0x192
    CONSUMER_EXPLORER = 0x194
    
    # Browser controls
    CONSUMER_BROWSER_HOME = 0x223
    CONSUMER_BROWSER_BACK = 0x224
    CONSUMER_BROWSER_FORWARD = 0x225
    CONSUMER_BROWSER_REFRESH = 0x227
    CONSUMER_BROWSER_BOOKMARKS = 0x22A

class SystemKeycode:
    """System control key codes for HID"""
    SYSTEM_POWER_DOWN = 0x81
    SYSTEM_SLEEP = 0x82
    SYSTEM_WAKE_UP = 0x83