#include "MatrixOS.h"

namespace MidiAction
{
    constexpr uint32_t signature = StaticHash("midi");

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        if(keyInfo->state != PRESSED || keyInfo->state != RELEASED || keyInfo->state != AFTERTOUCH ) return false;

        uint8_t data[3];

        for(uint8_t i = 1; i < actionData->length; i++)
        {
            cb0r_s cbor_data;
            cb0r_get(actionData, i, &cbor_data);
            data[i] = actionData->value;
        }

        if((data[0] & 0xF0) == NoteOn)
        {
            if(data[1] >> 7)
            {
                data[2] = keyInfo->velocity;
            }
            else
            {
                data[2] = keyInfo->velocity == 0 ? 0 : 127;
            }

            if(keyInfo->state == AFTERTOUCH)
            {
                data[0] = EMidiStatus::AfterTouch + (data[0] & 0x0F); 
            }
        }
        else if(keyInfo->state != PRESSED) // Only note need to handle Release and After Touch.
        {
            return true;
        }

        MatrixOS::MIDI::Send(MidiPacket(0, (EMidiStatus)data[0], data[0], data[1], data[2]));
        return true;
    }
};