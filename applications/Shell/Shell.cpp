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

std::vector<UIButton> commonBarBtns; //Because the UI in the common bar need to exist after the function ends
void Shell::AddCommonBarInUI(UI* ui)
{
  commonBarBtns.clear();
  commonBarBtns.reserve(2); //Make sure to change this after adding more stuffs, if vector content got relocated, UI will not be able to find it and will hardfault!
  commonBarBtns.push_back(UIButton("Application Launcher", Color(0x00FFAA), [&]() -> void {
    if (current_page != 0)
    {
      current_page = 0;
      ui->Exit();
    }
  }));
  ui->AddUIComponent(commonBarBtns.back(), Point(0, 7));

    #if MATRIXOS_LOG_LEVEL == LOG_LEVEL_DEBUG //Logging Mode Indicator
        #define SHELL_SYSTEM_SETTING_COLOR Color(0xFFBF00)
    #elif MATRIXOS_LOG_LEVEL == LOG_LEVEL_VERBOSE
        #define SHELL_SYSTEM_SETTING_COLOR Color(0xFF007F)
    #else
        #define SHELL_SYSTEM_SETTING_COLOR Color(0xFFFFFF)
    #endif

  commonBarBtns.push_back(UIButton("System Setting", SHELL_SYSTEM_SETTING_COLOR,
                                  [&]() -> void { MatrixOS::SYS::OpenSetting(); }));
  ui->AddUIComponent(commonBarBtns.back(), Point(7, 7));
  ui->AllowExit(false);  // So nothing happens
}
    
namespace MatrixOS::SYS{void ExecuteAPP(uint32_t app_id); uint16_t GetApplicationCount();} //Use non exposed Matrix OS API

void Shell::ApplicationLauncher()
{
    UI applicationLauncher("Application Launcher", Color(0x00FFAA));

    applicationLauncher.disableExit = true;
    AddCommonBarInUI(&applicationLauncher);

    uint16_t app_count = MatrixOS::SYS::GetApplicationCount();
    MatrixOS::Logging::LogDebug("Shell", "%d apps detected", app_count);

    uint16_t visable_app_count = 0;
    for(uint8_t i = 0; i < app_count; i++)
    { 
        if(applications[i]->visibility) {visable_app_count ++;}
    }

    std::vector<UIButton> appBtns;
    appBtns.reserve(visable_app_count);
    for(uint8_t i = 0; i < app_count; i++)
    { 
        if(applications[i]->visibility)
        {
            uint8_t x = appBtns.size() % 8; 
            uint8_t y = appBtns.size() / 8;

            uint32_t app_id = applications[i]->id;
            string app_name = applications[i]->name;
            Color app_color = applications[i]->color;

            appBtns.push_back(UIButton(app_name, app_color,
                                       [&, app_id]() -> void { MatrixOS::SYS::ExecuteAPP(app_id); }));
            applicationLauncher.AddUIComponent(appBtns.back(), Point(x, y));
            MatrixOS::Logging::LogDebug("Shell", "App #%d %s-%s loaded.", appBtns.size() - 1, applications[i]->author.c_str(), applications[i]->name.c_str());
        }
        else
        {
            MatrixOS::Logging::LogDebug("Shell", "%s not visible, skip.", applications[i]->name.c_str());
        }
    }
    applicationLauncher.Start();
}