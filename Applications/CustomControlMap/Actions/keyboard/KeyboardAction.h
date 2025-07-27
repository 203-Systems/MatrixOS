#include "MatrixOS.h"

namespace KeyboardAction
{
    const char* TAG = "KeyboardAction";

    constexpr uint32_t signature = StaticHash("keyboard");

    struct KeyboardAction
    {
        uint8_t key;
        uint8_t user_keycode;
    };

    static bool LoadData(cb0r_t actionData, KeyboardAction* action)
    {
        cb0r_s cbor_data;
        if (!cb0r_get_check_type(actionData, 1, &cbor_data, CB0R_INT))
        {
            MLOGE(TAG, "Failed to get action data");
            return false;
        }
        action->key = cbor_data.value;
        if(action->key == 0) // User defined key
        {
            if (!cb0r_get_check_type(actionData, 2, &cbor_data, CB0R_INT))
            {
                MLOGE(TAG, "Failed to get user keycode");
                return false;
            }
            action->user_keycode = cbor_data.value;
        }
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

        uint8_t keycode = action.key;

        if(action.key == 0)
        {
            keycode = action.user_keycode;
        }

        if(keyInfo->state == KeyState::PRESSED)
        {
          MLOGV(TAG, "Sending key char %d", keycode);
          MatrixOS::HID::Keyboard::Press((KeyboardKeycode)keycode);
          return true;
        }
        else if(keyInfo->state == KeyState::RELEASED)
        {
            MLOGV(TAG, "Releasing key char %d", keycode);
            MatrixOS::HID::Keyboard::Release((KeyboardKeycode)keycode);
            return true;
        }
        return false;
    }
};