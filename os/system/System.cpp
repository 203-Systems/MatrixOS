#include "MatrixOS.h"
#include "System.h"
#include "applications/Setting/Setting.h"
#include "applications/BootAnimations/BootAnimations.h"
#include "applications/Applications.h"

extern const Application_Info applications[] = {REGISTERED_APP_INFOS Application_Info()}; //Add NULL at the end due to trailing comma

namespace MatrixOS::SYS
{   
    void ApplicationFactory(void* param)
    {
        // MatrixOS::Logging::LogInfo("Application Factory", "App ID %d", active_app_id);
        switch(active_app_id)
        {
            REGISTERED_APP_SWITCH
            // case StaticHash("203 Electronics-Performance Mode"):
            //     // MatrixOS::Logging::LogInfo("Application Factory", "Launching Performance Mode");
            //     active_app = new Performance();
            //     break;
            // case StaticHash("203 Electronics-REDACTED"):
            //     active_app = new REDACTED();
            //     break;
            case 0:
            default:
                //SHELL
                //Temp Use Performance
                // Logging::LogError("System", "Requested APP not available");
                active_app = new Shell();
                break;
        }
        active_app->Start();
    }
    
    void Supervisor(void* param)
    {
        active_app_task = xTaskCreateStatic(ApplicationFactory,"application",  APPLICATION_STACK_SIZE, NULL, 1, application_stack, &application_taskdef);
        while(true)
        {
            if(eTaskGetState(active_app_task) == eTaskState::eDeleted)
            {
                active_app_task = xTaskCreateStatic(ApplicationFactory,"application",  APPLICATION_STACK_SIZE, NULL, 1, application_stack, &application_taskdef);
            }
            DelayMs(100);
        }
    }

    void Init()
    {
        Device::DeviceInit();
        LoadVariables();

        USB::Init();
        KEYPAD::Init();
        LED::Init();

        inited = true; 

        Logging::LogInfo("System", "Matrix OS initialization complete");

        Logging::LogError("Logging", "This is an error log");
        Logging::LogWarning("Logging", "This is a warning log");
        Logging::LogInfo("Logging", "This is an info log");
        Logging::LogDebug("Logging", "This is a debug log");
        Logging::LogVerbose("Logging", "This is a verbose log");

        MatrixBoot().Start(); //TODO Boot Animation Manager
        LED::Fill(0);
        LED::Update();

        // active_app_id = GenerateAPPID("203 Electronics", "Performance Mode");
        ExecuteAPP(active_app_id);
        (void) xTaskCreateStatic(Supervisor, "supervisor",  configMINIMAL_STACK_SIZE, NULL, 1, supervisor_stack, &supervisor_taskdef);
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
        // uint8_t x = 8; //TODO, fix this after GetDeviceInfo();
        // uint8_t y = 8;
        // if(x >= 4 && y >= 4)
        // {
        //     uint8_t x_center = x / 2;
        //     uint8_t y_center = y / 2;
        //     Color color = Color(0xFF0000);
        //     LED::SetColor(Point(x_center - 1, y_center - 2), color);
        //     LED::SetColor(Point(x_center, y_center - 2), color);
        //     LED::SetColor(Point(x_center - 2, y_center - 1), color);
        //     LED::SetColor(Point(x_center - 1, y_center - 1), color);
        //     LED::SetColor(Point(x_center, y_center - 1), color);
        //     LED::SetColor(Point(x_center + 1, y_center - 1), color);
        //     LED::SetColor(Point(x_center - 1, y_center), color);
        //     LED::SetColor(Point(x_center, y_center), color);
        //     LED::SetColor(Point(x_center - 1, y_center + 1), color);
        //     LED::SetColor(Point(x_center, y_center + 1), color);
        // }
        // // DelayMs(10);
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
        MatrixOS::Logging::LogDebug("System", "Set Variable [%s, %d]", variable, value);
        // MatrixOS::Logging::LogDebug("System", "Set Variable");
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
        // ESP_LOGI("System".c_str(), "Next Brightness");
        MatrixOS::Logging::LogDebug("System", "Next Brightness");
        uint8_t current_brightness = (uint8_t)GetVariable("brightness");
        // ESP_LOGI("System".c_str(), "Current Brightness %d", current_brightness);
        MatrixOS::Logging::LogDebug("System", "Current Brightness %d", current_brightness);
        for (uint8_t brightness: Device::brightness_level)
        {
            // ESP_LOGI("System".c_str(), "Check Brightness Level  %d", brightness);
            MatrixOS::Logging::LogDebug("System", "Check Brightness Level  %d", brightness);
            if (brightness > current_brightness)
            {
                // ESP_LOGI("System".c_str(), "Brightness Level Selected");
                MatrixOS::Logging::LogDebug("System", "Brightness Level Selected");
                MatrixOS::SYS::SetVariable("brightness", brightness);
                return;
            }
        }
        // ESP_LOGI("System".c_str(), "Lowest Level Selected");
        MatrixOS::Logging::LogDebug("System", "Lowest Level Selected");
        MatrixOS::SYS::SetVariable("brightness", Device::brightness_level[0]);
    }

    // void RegisterActiveApp(Application* application)
    // {
    //     SysVar::active_app = application;
    // }

    uint32_t GenerateAPPID(string author, string app_name)
    {
        // uint32_t app_id = Hash(author + "-" + app_name);
        // Logging::LogInfo("System", "APP ID: %u", app_id);
        return Hash(author + "-" + app_name);;
    }

    void ExecuteAPP(uint32_t app_id)
    {
        // Logging::LogInfo("System", "Launching APP ID\t%u", app_id);
        active_app_id = app_id;
        LED::Fill(0);
        LED::Update();
        if(active_app_task != NULL)
        {
            vTaskDelete(active_app_task);
            free(active_app);
            active_app = NULL;
        }
    }

    void ExecuteAPP(string author, string app_name)
    {
        Logging::LogInfo("System", "Launching APP\t%s - %s", author.c_str(), app_name.c_str());
        ExecuteAPP(GenerateAPPID(author, app_name));
    }

    void ExitAPP()
    {
        ExecuteAPP(0);
    }


    void ErrorHandler(string error)
    {
        // Bootloader();
        if(error.empty())
            error = "Undefined Error";
        Logging::LogError("System", "Matrix OS Error: %s", error);

        
        
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