#include "MatrixOS.h"

namespace KeyboardAction
{
    const char* TAG = "KeyboardAction";

    constexpr uint32_t signature = StaticHash("keyboard");

    struct KeyboardAction
    {
        uint8_t key;
    };

    static bool LoadData(cb0r_t actionData, KeyboardAction* action)
    {
        cb0r_s cbor_data;
        if(!cb0r_get(actionData, 1, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data");
            return false;
        }
        action->key = cbor_data.value;
        return true;
    }
    

    static bool KeyEvent(UADRuntime* uadRT, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        MLOGV(TAG, "KeyEvent");
        if(keyInfo->state != KeyState::PRESSED && keyInfo->state != KeyState::RELEASED) return false;

        struct KeyboardAction action;

        if(!LoadData(actionData, &action))
        {
            MLOGE(TAG, "Failed to load action");
            return false;
        }

        if(keyInfo->state == KeyState::PRESSED)
        {
            MLOGV(TAG, "Sending key char %d", action.key);
            MatrixOS::HID::Keyboard::Press((KeyboardKeycode)action.key);
            return true;
        }
        else if(keyInfo->state == KeyState::RELEASED)
        {
            MLOGV(TAG, "Releasing key char %d", action.key);
            MatrixOS::HID::Keyboard::Release((KeyboardKeycode)action.key);
            return true;
        }

        
        return false;
    }
};