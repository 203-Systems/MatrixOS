import _MatrixOS_MidiPacket
from _MatrixOS_MidiPacket import MidiStatus, MidiPortID

class MidiPacket(_MatrixOS_MidiPacket.MidiPacket):
    # def __init__(self, status: int, channel: int = 0, data1: int = 0, data2: int = 0):
    #     super().__init__(status, channel, data1, data2)
    
    # Static factory methods
    def NoteOn(channel: int, note: int, velocity: int = 127) -> MidiPacket:
        return super().NoteOn(channel, note, velocity)
    
    def NoteOff(channel: int, note: int, velocity: int = 0) -> MidiPacket:
        return super().NoteOff(channel, note, velocity)
    
    # def AfterTouch(channel: int, note: int, pressure: int) -> MidiPacket:
    #     return super().AfterTouch(channel, note, pressure)
    
    # def ControlChange(channel: int, controller: int, value: int) -> MidiPacket:
    #     return super().ControlChange(channel, controller, value)
    
    # def ProgramChange(channel: int, program: int) -> MidiPacket:
    #     return super().ProgramChange(channel, program)
    
    # def ChannelPressure(channel: int, pressure: int) -> MidiPacket:
    #     return super().ChannelPressure(channel, pressure)
    
    # def PitchBend(channel: int, value: int) -> MidiPacket:
    #     return super().PitchBend(channel, value)
    
    # def Clock() -> MidiPacket:
    #     return super().Clock()
    
    # def Start() -> MidiPacket:
    #     return super().Start()
    
    # def Continue() -> MidiPacket:
    #     return super().Continue()
    
    # def Stop() -> MidiPacket:
    #     return super().Stop()
    
    # def ActiveSense() -> MidiPacket:
    #     return super().ActiveSense()
    
    # def Reset() -> MidiPacket:
    #     return super().Reset()
    
    # def SongPosition(position: int) -> MidiPacket:
    #     return super().SongPosition(position)
    
    # def SongSelect(song: int) -> MidiPacket:
    #     return super().SongSelect(song)
    
    # def TuneRequest() -> MidiPacket:
    #     return super().TuneRequest()