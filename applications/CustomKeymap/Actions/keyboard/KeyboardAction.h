#include "MatrixOS.h"

namespace KeyboardAction
{
    constexpr uint32_t signature = StaticHash("keyboard");

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        if(keyInfo->state != KeyState::PRESSED || keyInfo->state != KeyState::RELEASED) return false;

        // TODO
        
        return true;
    }
};