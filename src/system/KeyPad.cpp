#include "MatrixOS.h"

namespace MatrixOS::KEYPAD
{
    KeyInfo* changelist[MULTIPRESS];

    void Init()
    {
        
    }

    void (*handler)(KeyInfo) = nullptr;

    void SetHandler(void (*handler)(KeyInfo))
    {
        KEYPAD::handler = handler;
    }
}