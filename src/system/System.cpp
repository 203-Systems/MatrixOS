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

    Timer ledTimer; 
    Timer keypadTimer; 
    void SystemTask()
    {
        USB::Poll();
        if(SysVar::led_update && ledTimer.Tick(SysVar::fps_millis)) //62.5 FPS
        {
            LED::Update();
        }
        if(SysVar::keypad_scan && keypadTimer.Tick(SysVar::keypad_millis)) //100HZ
        {
            KEYPAD::Scan();
        }
        // USB::MIDI::Poll();
        // USB::CDC::Poll();
        Device::DeviceTask();
    }

    uintptr_t GetAttribute(ESysVar variable)
    {
        switch(variable)
        {
            default:
                // _ASSERT(0);
                return (uintptr_t)nullptr;
        }
    }

    int8_t SetAttribute(ESysVar variable, uintptr_t value)
    {
        switch(variable)
        {
            case ESysVar::Rotation:
                UserVar::rotation = (EDirection)value;
                break;
            case ESysVar::Brightness:
                UserVar::brightness = (uint8_t)value;
                break;
            default:
                return -1;
        }
        return 0;
    }

    void RegisterActiveApp(Application* application)
    {
        SysVar::active_app = application;
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