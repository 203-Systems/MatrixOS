#include "MatrixOS.h"
#include "SavedVariable.h"
#include "application/Setting/Setting.h"

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
        LED::Fill(0);
        uint8_t x = GetVariable(ESysVar::MatrixSizeX);
        uint8_t y = GetVariable(ESysVar::MatrixSizeY);
        if(x >= 4 && y >= 4)
        {
            uint8_t x_center = x / 2;
            uint8_t y_center = y / 2;
            Color color = Color(0xFF0000);
            LED::SetColor(Point(x_center - 1, y_center - 2), color);
            LED::SetColor(Point(x_center, y_center - 2), color);
            LED::SetColor(Point(x_center - 2, y_center - 1), color);
            LED::SetColor(Point(x_center - 1, y_center - 1), color);
            LED::SetColor(Point(x_center, y_center - 1), color);
            LED::SetColor(Point(x_center + 1, y_center - 1), color);
            LED::SetColor(Point(x_center - 1, y_center), color);
            LED::SetColor(Point(x_center, y_center), color);
            LED::SetColor(Point(x_center - 1, y_center + 1), color);
            LED::SetColor(Point(x_center, y_center + 1), color);
        }
        // DelayMs(10);
        LED::Update();
        DelayMs(10); //Wait for led data to be updated first. TODO: Update with LED::updated var from device layer
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

    void OpenSetting(void)
    {
        Setting setting;
        setting.Start();
    }

    uintptr_t GetVariable(ESysVar variable)
    {
        switch(variable)
        {   
            case ESysVar::MatrixSizeX:
                return Device::x_size;
            case ESysVar::MatrixSizeY:
                return Device::y_size;
            case ESysVar::Rotation:
                return (uintptr_t)(&UserVar::rotation);
            case ESysVar::Brightness:
                return (uintptr_t)(&UserVar::brightness);
            default:
                // _ASSERT(0);
                return (uintptr_t)nullptr;
        }
    }

    int8_t SetVariable(ESysVar variable, uint32_t value)
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