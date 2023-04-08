#include "MatrixOS.h"

#define TAG "MidiAction"

namespace MidiAction
{
    constexpr uint32_t signature = StaticHash("midi");

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        MLOGV(TAG, "KeyEvent - Data Length: %d", actionData->length);
        if(keyInfo->state != PRESSED && keyInfo->state != RELEASED && keyInfo->state != AFTERTOUCH ) 
        {
            MLOGV(TAG, "Not useful");
            return false;
        }

        uint8_t data[3];
        
        for(uint8_t i = 1; i < actionData->length; i++)
        {
            cb0r_s cbor_data;
            if(!cb0r_get(actionData, i, &cbor_data) || cbor_data.type != CB0R_INT)
            {
                MLOGE(TAG, "Failed to get action data %d", i - 1);
                return false;
            }
            data[i - 1] = cbor_data.value;
        }

        // MLOGV(TAG, "Data: %d, %d, %d", data[0], data[1], data[2]);
        if((data[0] & 0xF0) == NoteOn)
        {
            // data[1] = bit 0-6 is note, bit 7 is a bool of use velocity from the keypad
            // data[2] = velocity
            if(data[1] >> 7)
            {
                data[1] &= 0x7F;
                data[2] = keyInfo->velocity;
                if(keyInfo->state == AFTERTOUCH)
                {
                    data[0] = EMidiStatus::AfterTouch + (data[0] & 0x0F); 
                }
            }
            else
            { 
                data[2] *= keyInfo->active();  // Use data[2] as velocity
            }
        }
        else if(keyInfo->state != PRESSED) // Only note need to handle Release and After Touch.
        {
            return true;
        }
        
        MLOGD(TAG, "Midi Sent %d %d %d %d", (EMidiStatus)data[0], data[0], data[1], data[2]);
        MatrixOS::MIDI::Send(MidiPacket(0, (EMidiStatus)(data[0] & 0xF0), data[0] & 0x0F, data[1], data[2]));
        return true;
    }
};