#include "usb/MidiSpecs.h"
#include <stdarg.h>

enum MidiStatus : uint8_t {None = 0,
                           NoteOff = MIDIv1_NOTE_OFF,
                           NoteOn = MIDIv1_NOTE_ON,
                           AfterTouch = MIDIv1_AFTER_TOUCH,
                           ControlChange = MIDIv1_CONTROL_CHANGE,
                           ProgramChange = MIDIv1_PROGRAM_CHANGE,
                           ChannelPressure = MIDIv1_CHANNEL_PRESSURE,
                           PitchChange = MIDIv1_PITCH_WHEEL,
                           SongPosition = MIDIv1_SONG_POSITION_PTR,
                           SongSelect = MIDIv1_SONG_SELECT,
                           TuneRequest = MIDIv1_TUNE_REQUEST,
                           Sync = MIDIv1_CLOCK,
                           Tick = MIDIv1_TICK,
                           Start = MIDIv1_START,
                           Continue = MIDIv1_CONTINUE,
                           Stop = MIDIv1_STOP,
                           ActiveSense = MIDIv1_ACTIVE_SENSE,
                           Reset = MIDIv1_RESET,
                           SysexData = MIDIv1_SYSEX_START,
                           SysexEnd = MIDIv1_SYSEX_END
                            };


struct MidiPacket
{
    uint16_t port;
    MidiStatus status;
    uint16_t length;
    uint8_t* data;

    MidiPacket(MidiStatus status, ...)
    {   
        va_list valst;
        MidiPacket(0, status, valst); 
    }

    MidiPacket(uint16_t port, MidiStatus status, ...)
    {
        this->port = port;
        this->status = status;
        va_list valst;
        va_start(valst, status);
        switch (status)
        {
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
                length = 3;
                data = (uint8_t*)malloc(3); 
                data[0] = (uint8_t)(status | ((uint8_t)va_arg(valst, int) & 0x0f));
                data[1] = (uint8_t)va_arg(valst, int);
                data[2] = (uint8_t)va_arg(valst, int);
                break;
            case ProgramChange:
            case ChannelPressure:
                length = 2;
                data = (uint8_t*)malloc(2); 
                data[0] = (uint8_t)(status | ((uint8_t)va_arg(valst, int) & 0x0f));
                data[1] = (uint8_t)va_arg(valst, int);
                break;
            case PitchChange:
            {
                length = 3;
                data = (uint8_t*)malloc(3); 
                data[0] = (uint8_t)(status | ((uint8_t)va_arg(valst, int) & 0x0f));
                uint16_t pitch = (uint16_t)va_arg(valst, int);
                data[1] = (uint8_t)(pitch & 0x07F);
                data[2] = (uint8_t)((pitch>>7) & 0x7f);
            }
                break;
            case SongSelect:
                length = 1;
                data = (uint8_t*)malloc(2); 
                data[0] = SongSelect;
                data[1] = (uint8_t)va_arg(valst, int);
                break;
            case SongPosition:
            {
                length = 3;
                data = (uint8_t*)malloc(3); 
                data[0] = SongPosition;
                uint16_t position = (uint16_t)va_arg(valst, int);
                data[1] = (uint8_t)(position & 0x07F);
                data[2] = (uint8_t)((position>>7) & 0x7f);
            }
            case TuneRequest:
            case Sync:
            case Start:
            case Continue:
            case Stop:
            case ActiveSense:
            case Reset:
                length = 0;
                break;
            case SysexData:
                //TODO 
                break;
            case None:
            default:
                length = 0;
                break;
        }
    }

    MidiPacket(MidiStatus status, uint16_t length, uint8_t* data = NULL) //I can prob use status to figure out length and assign it automaticlly
    {
        this->port = 0;
        this->status = status;
        this->length = length;
        this->data = (uint8_t*)malloc(length); //Malloc(0) is fine, not gonna bother checking.
        memcpy(this->data, data, length);
    }

    MidiPacket(uint16_t port, MidiStatus status, uint16_t length, uint8_t* data = NULL) //I can prob use status to figure out length and assign it automaticlly
    {
        this->port = port;
        this->status = status;
        this->length = length;
        this->data = (uint8_t*)malloc(length); //Malloc(0) is fine, not gonna bother checking.
        memcpy(this->data, data, length);
    }

    ~MidiPacket()
    {
        free(data); //free(NULL) is fine
    }
};