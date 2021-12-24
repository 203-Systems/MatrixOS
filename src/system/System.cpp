#include "MatrixOS.h"
#include "application/Setting/Setting.h"

namespace MatrixOS::SYS
{   
    void LoadVariables();
    void SaveVariables();

    string logTag = logTag;

    StaticTimer_t device_task_tmdef;
    TimerHandle_t device_task_tm;
    void Init()
    {
        Device::DeviceInit();
        // LoadVariables();

        USB::Init();
        KEYPAD::Init();
        LED::Init();

        // uint32_t brightness = 64;
        // bool r = Device::NVS::Write("U_brightness", &brightness, 4);
        // ESP_LOGI("Init", "Write Status %d", r);
        // vector<char> read = Device::NVS::Read("U_rotation");
        // ESP_LOGI("Init", "%s : %d : %d", "U_rotation", read.size(), *(uint32_t*)read.data());
        
        // (void) xTaskCreateStatic(SystemTask, "system task", SYS_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES-1, system_task_stack, &system_taskdef);
        
        // device_task_tm = xTimerCreateStatic(NULL, pdMS_TO_TICKS(1), true, NULL, Device::DeviceTask, &device_task_tmdef);
        // xTimerStart(device_task_tm, 0);

        inited = true; 
        // Logging::LogError(logTag, "This is an error log");
        // Logging::LogWarning(logTag, "This is a warning log");
        // Logging::LogInfo(logTag, "This is an info log");
        // Logging::LogDebug(logTag, "This is a debug log");
        // Logging::LogVerbose(logTag, "This is a verbose log");
    }
    
    uint32_t Millis() 
    {
        return ((((uint64_t) xTaskGetTickCount()) * 1000) / configTICK_RATE_HZ );
    }

    void DelayMs(uint32_t intervalMs)
    {
        vTaskDelay(pdMS_TO_TICKS(intervalMs));
    }

    void Reboot()
    {
        Device::Reboot();
    }

    void Bootloader()
    {
        LED::Fill(0);
        uint8_t x = 8; //TODO, fix this after GetDeviceInfo();
        uint8_t y = 8;
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

    void OpenSetting(void)
    {
        Setting setting;
        setting.Start();
    }

    void LoadVariables()
    {   
        //EEPROM was just reseted
        //EEPROM isn't initallized or Firmware was downgraded
        //EEPROM is at lower version (Delete variables that no longer needed)
        //EEPROM is at correct version
        for (auto& value : userVar) {
            vector<char> read = NVS::GetVariable("U_" + value.first);
            if(read.size() == 4)
            {
                value.second = *(uint32_t*)read.data();
            }
            MatrixOS::Logging::LogVerbose("UserVar", "%s : %d", ("U_" + value.first).c_str(), value.second);
        }
    }

    uint32_t GetVariable(string variable, EVarClass varClass)
    {   
        switch(varClass)
        {   
            case EVarClass::DeviceVar:
                return 0; //TODO
            case EVarClass::SystemVar:
                return 0; //TODO
            case EVarClass::UserVar:
                if(userVar.find(variable) != userVar.end()) //Check User Variables First
                    return userVar[variable];
                return 0;
            case EVarClass::AppVar:
                return 0;
        }
        return 0;
    }

    int8_t SetVariable(string variable, uint32_t value)
    {   
        //Save Variable
        MatrixOS::Logging::LogDebug(logTag, "Set Variable [%s, %d]", variable, value);
        // MatrixOS::Logging::LogDebug(logTag, "Set Variable");
        if(userVar.find(variable) != userVar.end()) //Check User Variables First
        {   
            userVar[variable] = value;
            NVS::SetVariable("U_" + variable, &value, 4);
        }
        else
        {
            return 0;
        }
        return 0;
    }

    void Rotate(EDirection rotation, bool absolute)
    {
        if(rotation == 90 || rotation == 180 || rotation == 270)
        {
            LED::RotateCanvas(rotation);
            SetVariable("rotation", (GetVariable("rotation") + rotation * !absolute) % 360);
        }
    }

    void NextBrightness()
    {
        // ESP_LOGI(logTag.c_str(), "Next Brightness");
        MatrixOS::Logging::LogDebug(logTag, "Next Brightness");
        uint8_t current_brightness = (uint8_t)GetVariable("brightness");
        // ESP_LOGI(logTag.c_str(), "Current Brightness %d", current_brightness);
        MatrixOS::Logging::LogDebug(logTag, "Current Brightness %d", current_brightness);
        for (uint8_t brightness: brightness_level)
        {
            // ESP_LOGI(logTag.c_str(), "Check Brightness Level  %d", brightness);
            MatrixOS::Logging::LogDebug(logTag, "Check Brightness Level  %d", brightness);
            if (brightness > current_brightness)
            {
                // ESP_LOGI(logTag.c_str(), "Brightness Level Selected");
                MatrixOS::Logging::LogDebug(logTag, "Brightness Level Selected");
                MatrixOS::SYS::SetVariable("brightness", brightness);
                return;
            }
        }
        // ESP_LOGI(logTag.c_str(), "Lowest Level Selected");
        MatrixOS::Logging::LogDebug(logTag, "Lowest Level Selected");
        MatrixOS::SYS::SetVariable("brightness", brightness_level[0]);
    }

    // void RegisterActiveApp(Application* application)
    // {
    //     SysVar::active_app = application;
    // }

    void ErrorHandler(string error)
    {
        // Bootloader();
        if(error.empty())
            error = "Undefined Error";
        Logging::LogError(logTag, "Matrix OS Error: %s", error);

        
        
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

        // uint32_t CFSR = *(uint32_t *) 0xE000ED28;

        // for(uint8_t y = 0; y < 4; y++)
        // {
        //     for(uint8_t x = 0; x < 8; x++)
        //     {
        //         uint32_t mask = 1 << (31 - (8 * y + x));
        //         LED::SetColor(Point(x,y + 4), 0xFFFFFF * ((CFSR & mask) > 0));
        //     }
        // }

        LED::Update();

        Device::ErrorHandler(); //Low level indicator in case LED and USB failed
    }
}