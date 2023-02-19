#include "MatrixOS.h"

namespace MidiAction
{
    const static uint32_t signature = StaticHash("midi");

    static bool KeyEvent(cb0r_t action, uint16_t keyID, KeyInfo* keyInfo)
    {
        if(keyInfo->state != KeyState::PRESSED || keyInfo->state != KeyState::RELEASED || keyInfo->state != KeyState::AFTERTOUCH ) return false;

        uint8_t data[3];

        for(uint8_t i = 1; i < action->length; i++)
        {
            cb0r_s cbor_data;
            cb0r_get(action, i, &cbor_data);
            data[i] = action->data[i];
        }

        if(data[0] & 0xF0 == NoteOn)
        {
            if(data[1] >> 7)
            {
                data[2] = keyInfo->velocity 
            }
            else
            {
                data[2] = keyInfo->velocity == 0 ? 0 : 127;
            }

            if(keyInfo->state == KeyState::AFTERTOUCH)
            {
                data[0] = EMidiStatus:AfterTouch + data[0] & 0x0F; 
            }
        }
        else if(keyInfo->state != KeyState::PRESSED) // Only note need to handle Release and After Touch.
        {
            return true;
        }

        MatrixOS::MIDI::Send(MidiPacket(0, data[0], data[0], data[1], data[2]));
        return true;
    }
};