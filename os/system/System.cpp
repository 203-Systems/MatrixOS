#include "MatrixOS.h"
#include "applications/Setting/Setting.h"
#include "applications/BootAnimations/BootAnimations.h"
#include "applications/Applications.h"
#include "System.h"

namespace MatrixOS::SYS
{   
    void ApplicationFactory(void* param)
    {
        MatrixOS::Logging::LogDebug("Application Factory", "App ID %X", active_app_id);

        active_app = NULL;

        if(active_app_id != 0)
        {
            for(uint8_t i = 0; i < app_count; i++) //I don't like the for loop but tbh there's nothing wrong with it.
            { 
                Application_Info* application = applications[i];
                if(application->id == active_app_id)
                {
                    MatrixOS::Logging::LogDebug("Application Factory", "Launching %s-%s", application->author.c_str(), application->name.c_str());
                    active_app = application->factory();
                    break;
                }
            }
        }
        
        if(active_app == NULL) //Default to launch shell
        {   
            if(active_app_id != 0)
                MatrixOS::Logging::LogDebug("Application Factory", "Can't find target app.");
            MatrixOS::Logging::LogDebug("Application Factory", "Launching Shell");
            active_app = new Shell();
        }

        active_app_id = 0; //Reset active_app_id so when active app exits it will default to shell again.
        active_app->Start();
    }
    
    void Supervisor(void* param)
    {

        MatrixOS::Logging::LogDebug("Supervisor", "%d Apps registered", app_count);

        for(uint8_t i = 0; i < app_count; i++)
        {
            Application_Info* application = applications[i];
            MatrixOS::Logging::LogDebug("Supervisor", "%X\t%s-%s v%u", application->id, application->author.c_str(), application->name.c_str(), application->version);
        }

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

        ExecuteAPP("203 Electronics", "Matrix Boot"); //Seperate boot animation with Application Class

        Device::PostBootTask(); //App won't run till supervisor is running

        (void) xTaskCreateStatic(Supervisor, "supervisor",  configMINIMAL_STACK_SIZE * 4, NULL, 1, supervisor_stack, &supervisor_taskdef);
        
        next_app = GenerateAPPID("203 Electronics", "Performance Mode"); //Launch Performance mode by default for now
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


    void Rotate(EDirection new_rotation, bool absolute)
    {
        if(new_rotation == 0 || new_rotation == 90 || new_rotation == 180 || new_rotation == 270)
        {
            if(new_rotation == 0 && !absolute) {return;}
            // LED::RotateCanvas(new_rotation); //TODO Does not work if absolute is true
            for(uint8_t ledLayer = 0; ledLayer < LED::CurrentLayer(); ledLayer++)
            {
                LED::Fill(0, ledLayer);
            }
            UserVar::rotation = (EDirection)((UserVar::rotation * !absolute + new_rotation ) % 360);
        }
    }

    void NextBrightness()
    {
        // ESP_LOGI("System".c_str(), "Next Brightness");
        MatrixOS::Logging::LogDebug("System", "Next Brightness");
        // ESP_LOGI("System".c_str(), "Current Brightness %d", current_brightness);
        MatrixOS::Logging::LogDebug("System", "Current Brightness %d", UserVar::brightness);
        for (uint8_t new_brightness: Device::brightness_level)
        {
            // ESP_LOGI("System".c_str(), "Check Brightness Level  %d", brightness);
            MatrixOS::Logging::LogDebug("System", "Check Brightness Level  %d", new_brightness);
            if (new_brightness > UserVar::brightness)
            {
                // ESP_LOGI("System".c_str(), "Brightness Level Selected");
                MatrixOS::Logging::LogDebug("System", "Brightness Level Selected");
                UserVar::brightness = new_brightness;
                return;
            }
        }
        // ESP_LOGI("System".c_str(), "Lowest Level Selected");
        MatrixOS::Logging::LogDebug("System", "Lowest Level Selected");
        UserVar::brightness = Device::brightness_level[0];
    }

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

        //Clean up layers that the previous app might have made
        while(LED::DestoryLayer()){}

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
        uint32_t app_id = next_app;
        next_app = 0;
        ExecuteAPP(app_id);
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

    uint16_t GetApplicationCount() //Used by shell, for some reason shell can not access app_count
    {
        return app_count;
    }
}