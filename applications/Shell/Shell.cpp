#include "Shell.h"

// void Shell::main()
// {
//     return;
// }

void Shell::Loop()
{
    switch(current_page)
    {
        case 0:
            ApplicationLauncher();
            break;
    }
}

void Shell::AddCommonBarInUI(UI* ui)
{
    ui->AddUIElement(new UIButton("Application Launcher", Color(0x00FFAA), [&]() -> void {if(current_page != 0) {current_page = 0; ui->Exit();}}), Point(0, 7));

    #if MATRIXOS_LOG_LEVEL == LOG_LEVEL_DEBUG //Logging Mode Indicator
        #define SHELL_SYSTEM_SETTING_COLOR Color(0xFFBF00)
    #elif MATRIXOS_LOG_LEVEL == LOG_LEVEL_VERBOSE
        #define SHELL_SYSTEM_SETTING_COLOR Color(0xFF007F)
    #else
        #define SHELL_SYSTEM_SETTING_COLOR Color(0xFFFFFF)
    #endif
    ui->AddUIElement(new UIButton("System Setting", SHELL_SYSTEM_SETTING_COLOR, [&]() -> void {MatrixOS::SYS::OpenSetting();}), Point(7, 7));
    ui->AllowExit(false); //So nothing happens
}
    
namespace MatrixOS::SYS{void ExecuteAPP(uint32_t app_id); uint16_t GetApplicationCount();} //Use non exposed Matrix OS API

void Shell::ApplicationLauncher()
{
    UI applicationLauncher("Application Launcher", Color(0x00FFAA));

    applicationLauncher.disableExit = true;
    AddCommonBarInUI(&applicationLauncher);

    uint16_t app_count = MatrixOS::SYS::GetApplicationCount();
    MatrixOS::Logging::LogDebug("Shell", "%d apps detected", app_count);

    uint16_t rendered_app_count = 0;
    for(uint8_t i = 0; i < app_count; i++) //I don't like the for loop but tbh there's nothing wrong with it.
    { 
        if(applications[i]->visibility)
        {
            uint8_t x = rendered_app_count % 8; 
            uint8_t y = rendered_app_count / 8;

            uint32_t app_id = applications[i]->id;
            string app_name = applications[i]->name;
            Color app_color = applications[i]->color;

            applicationLauncher.AddUIElement(new UIButton(app_name, 
                                    app_color, 
                                    [&, app_id]() -> void {MatrixOS::SYS::ExecuteAPP(app_id);}), 
                                    Point(x, y));
            MatrixOS::Logging::LogDebug("Shell", "App #%d %s-%s loaded.", rendered_app_count, applications[i]->author.c_str(), applications[i]->name.c_str());
            rendered_app_count ++;
        }
        else
        {
            MatrixOS::Logging::LogDebug("Shell", "%s not visible, skip.", applications[i]->name.c_str());
        }
    }
    applicationLauncher.Start();
}