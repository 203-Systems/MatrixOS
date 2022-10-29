#include "usb/MidiSpecs.h"
#include <stdarg.h>
// #include "esp_log.h"

enum EMidiStatus : uint8_t {None = 0,
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
    EMidiStatus status;
    // uint16_t length;
    uint8_t data[3] = {0, 0, 0};

    MidiPacket(EMidiStatus status, ...)
    {   
        // ESP_LOGI("Midi Packet", "Constructor 1");
        va_list valst;
        MidiPacket(0, status, valst); 
    }

    MidiPacket(uint16_t port, EMidiStatus status, ...)
    {
        // ESP_LOGI("Midi Packet", "Constructor 2");
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
                // length = 3;
                // data = (uint8_t*)pvPortMalloc(3); 
                data[0] = (uint8_t)(status | ((uint8_t)va_arg(valst, int) & 0x0f));
                data[1] = (uint8_t)va_arg(valst, int);
                data[2] = (uint8_t)va_arg(valst, int);
                break;
            case ProgramChange:
            case ChannelPressure:
                // length = 2;
                // data = (uint8_t*)pvPortMalloc(2); 
                data[0] = (uint8_t)(status | ((uint8_t)va_arg(valst, int) & 0x0f));
                data[1] = (uint8_t)va_arg(valst, int);
                break;
            case PitchChange:
            {
                // length = 3;
                // data = (uint8_t*)pvPortMalloc(3); 
                data[0] = (uint8_t)(status | ((uint8_t)va_arg(valst, int) & 0x0f));
                uint16_t pitch = (uint16_t)va_arg(valst, int);
                data[1] = (uint8_t)(pitch & 0x07F);
                data[2] = (uint8_t)((pitch>>7) & 0x7f);
            }
                break;
            case SongSelect:
                // length = 1;
                // data = (uint8_t*)pvPortMalloc(2); 
                data[0] = SongSelect;
                data[1] = (uint8_t)va_arg(valst, int);
                break;
            case SongPosition:
            {
                // length = 3;
                // data = (uint8_t*)pvPortMalloc(3); 
                data[0] = SongPosition;
                uint16_t position = (uint16_t)va_arg(valst, int);
                data[1] = (uint8_t)(position & 0x07F);
                data[2] = (uint8_t)((position>>7) & 0x7f);
                break;
            }
            case TuneRequest:
            case Sync:
            case Start:
            case Continue:
            case Stop:
            case ActiveSense:
            case Reset:
                // length = 0;
                break;
            case SysexData:
                //TODO 
                break;
            case None:
            default:
                // length = 0;
                break;
        }
    }

    MidiPacket(EMidiStatus status, uint16_t length, uint8_t* data) //I can prob use status to figure out length and assign it automaticlly
    {
        // ESP_LOGI("Midi Packet", "Constructor 3");
        MidiPacket(0, status, length, data);
    }

    MidiPacket(uint16_t port, EMidiStatus status, uint16_t length, uint8_t* data) //I can prob use status to figure out length and assign it automaticlly
    {
        // ESP_LOGI("Midi Packet", "Constructor 4");
        this->port = port;
        this->status = status;
        // this->length = length;
        // this->data = (uint8_t*)pvPortMalloc(length); //Malloc(0) is fine, not gonna bother checking.
        // ESP_LOGI("MP pre construct", "%#02X %#02X %#02X", data[0], data[1], data[2]);
        memcpy(this->data, data, length);
        // ESP_LOGI("MP post construct", "%#02X %#02X %#02X", this->data[0], this->data[1], this->data[2]);
        // ESP_LOGI("MP post construct2", "%#02X %#02X %#02X", this->channel(), this->note(), this->velocity());

    }

    uint8_t channel()
    {
        switch(status)
        {   
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
            case ProgramChange:
            case ChannelPressure:
            case PitchChange:
                return data[0] & 0x0F;
            default:
                return 0;
        }
    }

    uint8_t note()
    {
        switch(status)
        {   
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange: //To be honest this shouldn't be here but close enough
            case ProgramChange: //To be honest this shouldn't be here but close enough
                return data[1];
            default:
                return 0;
        }
    }

    uint8_t controller() //Just an alies for note(), specially build for Program Change
    {
        return note();
    }

    uint8_t velocity()
    {   
        switch(status)
        {   
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange: //Close Enough
                return data[2];
            case ChannelPressure:
                return data[1];
            default:
                return 0;
        }
    }

    uint16_t value() //Get value all type, basiclly a generic getter
    {
        switch(status)
        {   
            case NoteOn: //Close enough
            case NoteOff:
            case AfterTouch:
            case ControlChange: 
                return data[2];
            case ProgramChange:
            case ChannelPressure:
                return data[1];
            case PitchChange:
                return ((uint16_t)data[2] << 7) & data[1];
            case SongPosition:
                return ((uint16_t)data[1] << 7) & data[0];
            case SongSelect:
                return data[0];
            default:
                return 0;
        }
    }


    // ~MidiPacket()
    // {
        // ESP_LOGI("Midi Packet", "Free %d %p", (int)status, data);
        // free(data); //free(NULL) is fine
    // }

    uint8_t Length()
    {
        switch (status)
        {
            case NoteOn:
            case NoteOff:
            case AfterTouch:
            case ControlChange:
            case PitchChange:
            case SongPosition:
                return 3;
            case ProgramChange:
            case ChannelPressure:
                return 2;

            case SongSelect:
                return 1;
            case TuneRequest:
            case Sync:
            case Start:
            case Continue:
            case Stop:
            case ActiveSense:
            case Reset:
            case SysexData: //TODO
            case None:
            default:
                return 0;
        }
    }
};