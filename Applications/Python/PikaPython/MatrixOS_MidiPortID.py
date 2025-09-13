# MIDI Port ID enumeration  
class MidiPortID:
    MIDI_PORT_EACH_CLASS: int = 0x0
    MIDI_PORT_ALL: int = 0x01
    MIDI_PORT_USB: int = 0x100
    MIDI_PORT_PHYSICAL: int = 0x200
    MIDI_PORT_BLUETOOTH: int = 0x300
    MIDI_PORT_WIRELESS: int = 0x400
    MIDI_PORT_RTP: int = 0x500
    MIDI_PORT_DEVICE_CUSTOM: int = 0x600
    MIDI_PORT_SYNTH: int = 0x8000
    MIDI_PORT_INVALID: int = 0xFFFF
