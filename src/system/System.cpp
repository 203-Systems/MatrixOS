#include "MatrixOS.h"

namespace MatrixOS::SYS
{
    void Init()
    {
        Device::DeviceInit();
        MatrixOS::KEYPAD::Init();
        MatrixOS::LED::Init();
        MatrixOS::USB::Init();

        inited = true;
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

    Timer keypadTimer; 
    void SystemTask()
    {
        // if(keypadTimer.Tick(10))
        // {
        //     KEYPAD::Scan();
        // }
        USB::Poll();
        // USB::MIDI::Poll();
        // USB::CDC::Poll();
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
        if(error == NULL)
            USB::CDC::Println("Undefined Error");
        else
            USB::CDC::Println(error);
        
        //Show Blue Screen
        LED::Fill(0x00adef);
        if(Device::x_size >= 5 && Device::y_size >= 5)
        {
            LED::SetColor(Point(1,1), 0xFFFFFF);
            LED::SetColor(Point(1,3), 0xFFFFFF);

            LED::SetColor(Point(3,1), 0xFFFFFF);
            LED::SetColor(Point(3,2), 0xFFFFFF);
            LED::SetColor(Point(3,3), 0xFFFFFF);
        }
        LED::Update();

        Device::ErrorHandler(); //Low level indicator in case LED and USB failed
    }
}