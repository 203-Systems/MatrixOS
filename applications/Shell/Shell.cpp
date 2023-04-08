#include "Shell.h"

void Shell::Loop() {
  switch (current_page)
  {
    case 0:
      ApplicationLauncher();
      break;
  }
}

std::vector<UIButton> commonBarBtns;  // Because the UI in the common bar need to exist after the function ends
uint8_t tap_counter = 0;
uint32_t last_tap = 0;

void Shell::AddCommonBarInUI(UI* ui) {
  commonBarBtns.clear();
  commonBarBtns.reserve(2);  // Make sure to change this after adding more stuffs, if vector content got relocated, UI
                             // will not be able to find it and will hardfault!
  commonBarBtns.push_back(UIButtonDimmable(
      "Application Launcher", Color(0x00FFAA), [&]() -> bool { return current_page == 0; },
      [&]() -> void {
        if (current_page != 0)
        {
          current_page = 0;
          ui->Exit();
        }
        else 
        {
          // Tap on the application launcher button 10 times to show hidden apps
          if (MatrixOS::SYS::Millis() - last_tap < 1000)
          {
            tap_counter++;
          }

          last_tap = MatrixOS::SYS::Millis();

          // MLOGI("Hidden Launcher", "Tap %d", tap_counter);

          if (tap_counter >= 10)
          {
            tap_counter = 0;
            MLOGI("Hidden Launcher", "Enter");
            HiddenApplicationLauncher();
          }
        }
      }));
  ui->AddUIComponent(commonBarBtns.back(), Point(0, 7));

#if MATRIXOS_LOG_LEVEL == LOG_LEVEL_DEBUG  // Logging Mode Indicator
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

namespace MatrixOS::SYS
{
  void ExecuteAPP(uint32_t app_id);
  uint16_t GetApplicationCount();
}  // Use non exposed Matrix OS API

void Shell::ApplicationLauncher() {
  UI applicationLauncher("Application Launcher", Color(0x00FFAA));

  applicationLauncher.disableExit = true;
  AddCommonBarInUI(&applicationLauncher);

  uint16_t app_count = MatrixOS::SYS::GetApplicationCount();
  MLOGD("Shell", "%d apps detected", app_count);

  uint16_t visable_app_count = 0;

  // Iterate though map
  for (auto const& [app_id, application] : applications)
  {
    if (application->visibility)
    { visable_app_count++; }
  }

  std::vector<UIButton> appBtns;
  appBtns.reserve(visable_app_count);

  // Iterate though deque 
  for (uint32_t app_id: application_ids)
  {
    auto application_it = applications.find(app_id);
    if(application_it == applications.end())
    {
      continue;
    }
    Application_Info* application = application_it->second;

    if (application->visibility)
    {
      uint8_t x = appBtns.size() % 8;
      uint8_t y = appBtns.size() / 8;

      string app_name = application->name;
      Color app_color = application->color;

      appBtns.push_back(UIButton(app_name, app_color, [&, app_id]() -> void { MatrixOS::SYS::ExecuteAPP(app_id); }));
      applicationLauncher.AddUIComponent(appBtns.back(), Point(x, y));
      MLOGD("Shell", "App #%d %s-%s loaded.", appBtns.size() - 1, application->author.c_str(),
                                  application->name.c_str());
    }
    else
    { MLOGD("Shell", "%s not visible, skip.", application->name.c_str()); }
  }
  applicationLauncher.Start();
}

void Shell::HiddenApplicationLauncher() {
  UI hiddenApplicationLauncher("Hidden Application Launcher", Color(0xFFFFFF));

  uint16_t app_count = MatrixOS::SYS::GetApplicationCount();
  MLOGD("Shell", "%d apps detected", app_count);

  uint16_t invisable_app_count = 0;

  // Iterate though map
  for (auto const& [app_id, application] : applications)
  {
    if (application->visibility)
    { invisable_app_count++; }
  }

  std::vector<UIButton> appBtns;
  appBtns.reserve(invisable_app_count);
  for (uint32_t app_id: application_ids)
  {
    auto application_it = applications.find(app_id);
    if(application_it == applications.end())
    {
      continue;
    }
    Application_Info* application = application_it->second;

    if (application->visibility == false)
    {
      uint8_t x = appBtns.size() % 8;
      uint8_t y = appBtns.size() / 8;

      string app_name = application->name;
      Color app_color = application->color;

      appBtns.push_back(UIButton(app_name, app_color, [&, app_id]() -> void { MatrixOS::SYS::ExecuteAPP(app_id); }));
      hiddenApplicationLauncher.AddUIComponent(appBtns.back(), Point(x, y));
      MLOGD("Shell (invisable)", "App #%d %s-%s loaded.", appBtns.size() - 1, application->author.c_str(),
                                  application->name.c_str());
    }
    else
    { MLOGD("Shell", "%s visible, skip.", application->name.c_str()); }
  }
  hiddenApplicationLauncher.Start();
}