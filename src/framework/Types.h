#pragma once
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "usb/MidiSpecs.h"

#ifdef __cplusplus
extern "C" 
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef const uint8_t uc8;
typedef const uint16_t uc16;
typedef const uint32_t uc32;
typedef const uint64_t uc64;

typedef volatile uint8_t vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;
typedef volatile uint64_t vu64;

typedef volatile const uint8_t vuc8;
typedef volatile const uint16_t vuc16;
typedef volatile const uint32_t vuc32;
typedef volatile const uint64_t vuc64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef const int8_t sc8;
typedef const int16_t sc16;
typedef const int32_t sc32;
typedef const int64_t sc64;

typedef volatile int8_t vs8;
typedef volatile int16_t vs16;
typedef volatile int32_t vs32;
typedef volatile int64_t vs64;

typedef volatile const int8_t vsc8;
typedef volatile const int16_t vsc16;
typedef volatile const int32_t vsc32;
typedef volatile const int64_t vsc64;
typedef void* PVOID;

enum Direction {UP, RIGHT, DOWN, LEFT};

class Fract16
{
    public:
    uint16_t value;
    Fract16(uint32_t value)
    {
        this->value = (uint16_t)value;
    }

    Fract16(uint16_t value, uint8_t bits)
    {
        this->value = value << (16 - bits);
        // TODO: Fill the empty part of the 
        // uint16_t fill = value 
        // for(uint8_t i = 0; i < (16 - bits) / bits)
        // {

        // }
    }

    // uint8_t to14bits(){return value >> 2;}
    // uint8_t to12bits(){return value >> 4;}
    // uint8_t to10bits(){return value >> 6;}
    uint8_t to8bits(){return value >> 8;}
    uint8_t to7bits(){return value >> 9;}
    
    operator bool() {return value > 0;}
    operator uint8_t() {return to8bits();}
    operator uint16_t() {return value;}
    operator uint32_t() {return value;}

    bool operator <(int value) {return this->value < value;}
    bool operator >(int value) {return this->value > value;}

    bool operator ==(int value) {return this->value == value;}
};

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
    MidiStatus status;
    uint16_t length;
    uint8_t* data;

    MidiPacket(MidiStatus status, uint16_t length = 0, uint8_t* data = NULL) //I can prob use status to figure out length and assign it automaticlly
    {
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