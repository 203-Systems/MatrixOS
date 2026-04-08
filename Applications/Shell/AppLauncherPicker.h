
#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "Shell.h"

class AppLauncherPicker : public UIComponent {
public:
  Shell* shell;
  AppLauncherPicker(Shell* shell) {
    this->shell = shell;
  }

  virtual Dimension GetSize() {
    return Dimension(8, 7);
  }

  virtual bool Render(Point origin) {
    uint16_t addedApps = 0;
    for (uint8_t i = 0; i < shell->folders[shell->current_folder].app_ids.size(); i++)
    {
      uint32_t appId = shell->folders[shell->current_folder].app_ids[i];
      auto applicationIt = shell->all_applications.find(appId);
      if (applicationIt == shell->all_applications.end())
      {
        // Skip invalid app ID - should have been cleaned up on startup
        continue;
      }
      ApplicationEntry& applicationEntry = applicationIt->second;
      Application_Info* applicationInfo =
          (applicationEntry.type == ApplicationType::Native) ? applicationEntry.native.info : &(applicationEntry.python.info->info);

      uint8_t x = addedApps % 8;
      uint8_t y = addedApps / 8;

      Color appColor = applicationInfo->color;
      MatrixOS::LED::SetColor(origin + Point(x, y), appColor);
      addedApps++;

      if (addedApps >= 8 * 7)
      {
        break;
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) {
    if (keypadInfo->state == KeypadState::Released || keypadInfo->state == KeypadState::Hold)
    {
      uint8_t index = xy.y * 8 + xy.x;
      if (index >= shell->folders[shell->current_folder].app_ids.size())
      {
        return false;
      }

      uint32_t appId = shell->folders[shell->current_folder].app_ids[index];
      auto applicationIt = shell->all_applications.find(appId);
      if (applicationIt == shell->all_applications.end())
      {
        // Skip invalid app ID - should have been cleaned up on startup
        return false;
      }
      ApplicationEntry& applicationEntry = applicationIt->second;
      Application_Info* application =
          (applicationEntry.type == ApplicationType::Native) ? applicationEntry.native.info : &(applicationEntry.python.info->info);

      if (keypadInfo->state == KeypadState::Released)
      {
        MLOGD("Shell", "Launching App ID: %X", appId);
        shell->LaunchAnimation(xy, application->color);

        if (applicationEntry.type == ApplicationType::Python)
        {
          // Launch Python app with script path argument
          vector<string> args = {applicationEntry.python.info->file_path};
          MatrixOS::SYS::ExecuteAPP("203 Systems", "Python", args);
        }
        else
        {
          // Launch native app normally
          MatrixOS::SYS::ExecuteAPP(appId);
        }
        return true;
      }
      else if (keypadInfo->state == KeypadState::Hold)
      {
        MatrixOS::UIUtility::TextScroll(application->name, application->color);
        return true;
      }
    }
    return false;
  }
};