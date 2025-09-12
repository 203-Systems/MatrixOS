#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // MidiPacket constructor
    void _MatrixOS_MidiPacket_MidiPacket___init__(PikaObj *self, int status, int channel, int data1, int data2) {
        obj_setInt(self, (char*)"port", MIDI_PORT_INVALID);
        obj_setInt(self, (char*)"status", status);
        
        // Store raw data array
        obj_setInt(self, (char*)"data0", (status & 0xF0) | (channel & 0x0F));
        obj_setInt(self, (char*)"data1", data1);
        obj_setInt(self, (char*)"data2", data2);
    }

    // Factory methods for creating MIDI packets
    PikaObj* _MatrixOS_MidiPacket_MidiPacket_NoteOn(PikaObj *self, int channel, int note, int velocity) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, NoteOn, channel, note & 0x7F, velocity & 0x7F);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_NoteOff(PikaObj *self, int channel, int note, int velocity) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, NoteOff, channel, note & 0x7F, velocity & 0x7F);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_AfterTouch(PikaObj *self, int channel, int note, int pressure) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, AfterTouch, channel, note & 0x7F, pressure & 0x7F);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_ControlChange(PikaObj *self, int channel, int controller, int value) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, ControlChange, channel, controller & 0x7F, value & 0x7F);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_ProgramChange(PikaObj *self, int channel, int program) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, ProgramChange, channel, program & 0x7F, 0);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_ChannelPressure(PikaObj *self, int channel, int pressure) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, ChannelPressure, channel, pressure & 0x7F, 0);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_PitchBend(PikaObj *self, int channel, int value) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, PitchChange, channel, value & 0x7F, (value >> 7) & 0x7F);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Clock(PikaObj *self) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, Sync, 0, 0, 0);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Start(PikaObj *self) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, Start, 0, 0, 0);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Continue(PikaObj *self) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, Continue, 0, 0, 0);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Stop(PikaObj *self) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, Stop, 0, 0, 0);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_ActiveSense(PikaObj *self) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, ActiveSense, 0, 0, 0);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Reset(PikaObj *self) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, Reset, 0, 0, 0);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_SongPosition(PikaObj *self, int position) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, SongPosition, 0, position & 0x7F, (position >> 7) & 0x7F);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_SongSelect(PikaObj *self, int song) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, SongSelect, 0, song & 0x7F, 0);
        return self;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_TuneRequest(PikaObj *self) {
        _MatrixOS_MidiPacket_MidiPacket___init__(self, TuneRequest, 0, 0, 0);
        return self;
    }

    // Port getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Port(PikaObj *self) {
        return obj_getInt(self, (char*)"port");
    }
    
    // Status getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Status(PikaObj *self) {
        return obj_getInt(self, (char*)"status");
    }

    void _MatrixOS_MidiPacket_MidiPacket_SetStatus(PikaObj *self, int status) {
        obj_setInt(self, (char*)"status", status);
        // Update data[0] with new status
        int channel = obj_getInt(self, (char*)"data0") & 0x0F;
        obj_setInt(self, (char*)"data0", (status & 0xF0) | channel);
    }

    // Channel getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Channel(PikaObj *self) {
        int status = obj_getInt(self, (char*)"status");
        switch ((EMidiStatus)status) {
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
            case ProgramChange:
            case ChannelPressure:
            case PitchChange:
                return obj_getInt(self, (char*)"data0") & 0x0F;
            default:
                return 0;
        }
    }

    void _MatrixOS_MidiPacket_MidiPacket_SetChannel(PikaObj *self, int channel) {
        int status = obj_getInt(self, (char*)"status");
        int data0 = obj_getInt(self, (char*)"data0");
        obj_setInt(self, (char*)"data0", (status & 0xF0) | (channel & 0x0F));
    }

    // Note getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Note(PikaObj *self) {
        int status = obj_getInt(self, (char*)"status");
        switch ((EMidiStatus)status) {
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
            case ProgramChange:
                return obj_getInt(self, (char*)"data1");
            default:
                return 0;
        }
    }

    void _MatrixOS_MidiPacket_MidiPacket_SetNote(PikaObj *self, int note) {
        obj_setInt(self, (char*)"data1", note & 0x7F);
    }

    // Controller getter/setter (alias for note)
    int _MatrixOS_MidiPacket_MidiPacket_Controller(PikaObj *self) {
        return _MatrixOS_MidiPacket_MidiPacket_Note(self);
    }

    void _MatrixOS_MidiPacket_MidiPacket_SetController(PikaObj *self, int controller) {
        _MatrixOS_MidiPacket_MidiPacket_SetNote(self, controller);
    }

    // Velocity getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Velocity(PikaObj *self) {
        int status = obj_getInt(self, (char*)"status");
        switch ((EMidiStatus)status) {
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
                return obj_getInt(self, (char*)"data2");
            case ChannelPressure:
                return obj_getInt(self, (char*)"data1");
            default:
                return 0;
        }
    }

    void _MatrixOS_MidiPacket_MidiPacket_SetVelocity(PikaObj *self, int velocity) {
        int status = obj_getInt(self, (char*)"status");
        switch ((EMidiStatus)status) {
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
                obj_setInt(self, (char*)"data2", velocity & 0x7F);
                break;
            case ChannelPressure:
                obj_setInt(self, (char*)"data1", velocity & 0x7F);
                break;
            default:
                break;
        }
    }

    // Value getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Value(PikaObj *self) {
        int status = obj_getInt(self, (char*)"status");
        int data0 = obj_getInt(self, (char*)"data0");
        int data1 = obj_getInt(self, (char*)"data1");
        int data2 = obj_getInt(self, (char*)"data2");
        
        switch ((EMidiStatus)status) {
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
                return data2;
            case ProgramChange:
            case ChannelPressure:
                return data1;
            case PitchChange:
                return ((uint16_t)data2 << 7) | data1;
            case SongPosition:
                return ((uint16_t)data1 << 7) | data0;
            case SongSelect:
                return data0;
            default:
                return 0;
        }
    }

    void _MatrixOS_MidiPacket_MidiPacket_SetValue(PikaObj *self, int value) {
        int status = obj_getInt(self, (char*)"status");
        
        switch ((EMidiStatus)status) {
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
                obj_setInt(self, (char*)"data2", value & 0x7F);
                break;
            case ProgramChange:
            case ChannelPressure:
                obj_setInt(self, (char*)"data1", value & 0x7F);
                break;
            case PitchChange:
                obj_setInt(self, (char*)"data1", value & 0x7F);
                obj_setInt(self, (char*)"data2", (value >> 7) & 0x7F);
                break;
            case SongPosition:
                obj_setInt(self, (char*)"data0", value & 0x7F);
                obj_setInt(self, (char*)"data1", (value >> 7) & 0x7F);
                break;
            case SongSelect:
                obj_setInt(self, (char*)"data0", value & 0x7F);
                break;
            default:
                break;
        }
    }

    // Helper methods
    int _MatrixOS_MidiPacket_MidiPacket_Length(PikaObj *self) {
        int status = obj_getInt(self, (char*)"status");
        int data0 = obj_getInt(self, (char*)"data0");
        int data1 = obj_getInt(self, (char*)"data1");
        
        switch ((EMidiStatus)status) {
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
            case PitchChange:
            case SongPosition:
            case SysExData:
                return 3;
            case ProgramChange:
            case ChannelPressure:
            case SongSelect:
                return 2;
            case TuneRequest:
            case Sync:
            case Tick:
            case Start:
            case Continue:
            case Stop:
            case ActiveSense:
            case Reset:
                return 1;
            case SysExEnd:
                if (data0 == 0xF7)
                    return 1;
                else if (data1 == 0xF7)
                    return 2;
                else
                    return 3;
            case None:
            default:
                return 0;
        }
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SysEx(PikaObj *self) {
        int status = obj_getInt(self, (char*)"status");
        return (status == SysExData || status == SysExEnd) ? pika_true : pika_false;
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SysExStart(PikaObj *self) {
        int status = obj_getInt(self, (char*)"status");
        int data0 = obj_getInt(self, (char*)"data0");
        return (status == SysExData && data0 == 0xF0) ? pika_true : pika_false;
    }
}