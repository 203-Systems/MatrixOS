import _MatrixOS_MidiPacket
from MatrixOS_MidiPortID import MidiPortID
from MatrixOS_MidiStatus import MidiStatus

class MidiPacket(_MatrixOS_MidiPacket.MidiPacket):    
    # Static factory methods
    def NoteOn(self, channel: int, note: int, velocity: int = 127) -> MidiPacket:
        return super().NoteOn(channel, note, velocity)
    
    def NoteOff(self, channel: int, note: int, velocity: int = 0) -> MidiPacket:
        return super().NoteOff(channel, note, velocity)