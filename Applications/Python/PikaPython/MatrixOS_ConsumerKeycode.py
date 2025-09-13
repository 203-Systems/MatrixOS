# Consumer control key codes for MatrixOS HID
# Binding of OS/HID/HIDSpecs.h

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