#include "MatrixOS.h"

namespace MidiAction
{
    const static uint32_t signature = StaticHash("midi");

    static bool KeyEvent(cb0r_t action, uint16_t keyID, KeyInfo* keyInfo)
    {
        return true;
    }
};