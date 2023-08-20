#include "MatrixOS.h"

namespace MidiAction
{
    const char* TAG = "MidiAction";

    constexpr uint32_t signature = StaticHash("midi");

    struct MidiAction
    {
      uint8_t data0;
      uint8_t data1;
      uint8_t data2;
    };

    static bool LoadData(cb0r_t actionData, MidiAction* action)
    {
        cb0r_s cbor_data;
        if(!cb0r_get(actionData, 1, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data 0");
            return false;
        }
        action->data0 = cbor_data.value;

        if(!cb0r_get(actionData, 2, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data 1");
            return false;
        }
        action->data1 = cbor_data.value;

        if(!cb0r_get(actionData, 3, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data 2");
            return false;
        }
        action->data2 = cbor_data.value;
        return true;
    }
    

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        MLOGV(TAG, "KeyEvent - Data Length: %d", actionData->length);
        if(keyInfo->state != PRESSED && keyInfo->state != RELEASED && keyInfo->state != AFTERTOUCH ) 
        {
            MLOGV(TAG, "Not useful");
            return false;
        }

        struct MidiAction data;

        if(!LoadData(actionData, &data))
        {
            MLOGE(TAG, "Failed to load action");
            return false;
        }

        // MLOGV(TAG, "Data: %d, %d, %d", data.data0, data.data1, data.data2);
        if((data.data0 & 0xF0) == NoteOn)
        {
            // data.data1 = bit 0-6 is note, bit 7 is a bool of use velocity from the keypad
            // data.data2 = velocity
            if(data.data1 >> 7)
            {
                data.data1 &= 0x7F;
                data.data2 = keyInfo->velocity;
                if(keyInfo->state == AFTERTOUCH)
                {
                    data.data0 = EMidiStatus::AfterTouch + (data.data0 & 0x0F); 
                }
            }
            else
            { 
                data.data2 *= keyInfo->active();  // Use data.data2 as velocity
            }
        }
        else if(keyInfo->state != PRESSED) // Only note need to handle Release and After Touch.
        {
            return true;
        }
        
        MLOGD(TAG, "Midi Sent %d %d %d %d", (EMidiStatus)data.data0, data.data0, data.data1, data.data2);
        MatrixOS::MIDI::Send(MidiPacket(0, (EMidiStatus)(data.data0 & 0xF0), data.data0 & 0x0F, data.data1, data.data2));
        return true;
    }
};