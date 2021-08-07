#include "MatrixOS.h"

namespace MatrixOS::SYS
{
    void Init()
    {
        Device::DeviceInit();
        MatrixOS::KEYPAD::Init();
        MatrixOS::LED::Init();
        MatrixOS::USB::Init();
    }
    
    uint32_t Millis()
    {
        return Device::Millis();
    }

    void DelayMs(uint32_t intervalMs)
    {
        Device::Delay(intervalMs);
    }

    void Reboot()
    {
        Device::Reboot();
    }

    void Bootloader()
    {
        Device::Bootloader();
    }

    void SystemTask()
    {
        USB::Poll();
        USB::MIDI::Poll();
        USB::CDC::Poll();
        Device::DeviceTask();
    }

    uintptr_t GetAttribute(SysVar variable)
    {
        switch(variable)
        {
            default:
                // _ASSERT(0);
                return (uintptr_t)nullptr;
        }
    }

    int8_t SetAttribute(SysVar variable, uintptr_t value)
    {
        switch(variable)
        {
            case SysVar::Rotation:
                rotation = (uint8_t)value;
                break;
            case SysVar::Brightness:
                brightness = (uint8_t)value;
                break;
            default:
                return -1;
        }
        return 0;
    }

    void ErrorHandler(char const* error)
    {
        USB::CDC::Print("Matrix OS Error: ");
        USB::CDC::Println(error);
        //TODO Show Blue Screen

        Device::ErrorHandler(); //Low level indicator in case LED and USB failed
    }
}