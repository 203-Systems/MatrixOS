#include "MatrixOS.h"

namespace KeyboardAction
{
    constexpr uint32_t signature = StaticHash("keyboard");

    struct KeyboardAction
    {
        uint8_t key;
    };

    static bool LoadAction(cb0r_t actionData, KeyboardAction* action)
    {
        cb0r_s cbor_data;
        if(!cb0r_get(actionData, 1, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data %d", 1);
            return false;
        }
        action->key = cbor_data.value;
        return true;
    }
    

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        if(keyInfo->state != KeyState::PRESSED || keyInfo->state != KeyState::RELEASED) return false;

        struct KeyboardAction action;

        if(!LoadAction(actionData, &action))
        {
            MLOGE(TAG, "Failed to load action");
            return false;
        }

        if(keyInfo->state == KeyState::PRESSED)
        {
            MatrixOS::HID::Keyboard::Press((KeyboardKeycode)action.key);
            return true;
        }
        else if(keyInfo->state == KeyState::RELEASED)
        {
            MatrixOS::HID::Keyboard::Release((KeyboardKeycode)action.key);
            return true;
        }

        
        return false;
    }
};