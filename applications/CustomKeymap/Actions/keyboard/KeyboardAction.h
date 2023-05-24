#include "MatrixOS.h"

namespace KeyboardAction
{
    constexpr uint32_t signature = StaticHash("keyboard");

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        if(keyInfo->state != KeyState::PRESSED || keyInfo->state != KeyState::RELEASED) return false;

        cb0r_s cbor_data;
        if(!cb0r_get(actionData, 1, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data %d", i - 1);
            return false;
        }

        uint8_t keycode = cbor_data.value;

        if(keyInfo->state == KeyState::PRESSED)
        {
            MatrixOS::KEYPAD::Press(keycode);
        }
        else
        {
            MatrixOS::KEYPAD::Release(keycode);
        }

        return true;
    }
};