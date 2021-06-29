#include "MatrixOS.h"

namespace MatrixOS
{
    namespace SYS
    {
        void Init()
        {
            Device::Hardware_Init();
        }

        void DelayMs(uint32_t intervalMs)
        {
            Device::DelayMs(intervalMs);
        }

        void Reboot()
        {
            Device::Reboot();
        }

        void Bootloader()
        {
            Device::Bootloader();
        }

        uintptr_t GetAttribute(EAttribute eInternal)
        {
            return 0;
        }
        uint8_t SetAttribute(EAttribute eInternal, uint32_t value)
        {
            return 0;
        }
    }
    namespace LED
    {
        uint8_t currentLayer = 0;
        Color frameBuffer[NUMSOFLEDLAYER][NUMSOFLED];
    }
    namespace KEYPAD
    {
        KeyInfo* changelist[MULTIPRESS];
    }
}