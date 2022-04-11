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
    ui->AddUIElement(UIElement("Application Launcher", Color(0x00FFAA), [&]() -> void {if(current_page != 0) {current_page = 0; ui->Exit();}}), Point(0, 7));
    ui->AddUIElement(UIElement("System Setting", Color(0xFFFFFF), [&]() -> void {MatrixOS::SYS::OpenSetting();}), Point(7, 7));
}

extern Application_Info applications[];
void Shell::ApplicationLauncher()
{
    UI applicationLauncher("Application Launcher", Color(0x00FFAA));

    AddCommonBarInUI(&applicationLauncher);

    for(uint8_t i = 0; i < 2; i++) // Not supporting more apps atm
    {
        uint8_t x = i % 8;
        uint8_t y = i / 8;
        string app_author = applications[i].author;
        string app_name = applications[i].name;
        applicationLauncher.AddUIElement(UIElement(applications[i].name, \
                                   applications[i].color, \
                                   [&, app_author, app_name]() -> void {MatrixOS::SYS::ExecuteAPP(app_author, app_name);}), \
                                   Point(x, y));
    }

    applicationLauncher.Start();
}