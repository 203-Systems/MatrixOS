#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "Shell.h"

class AppLauncherPickerEditMode : public UIComponent {
public:
  Shell* shell;

  AppLauncherPickerEditMode(Shell* shell) {
    this->shell = shell;
  }

  void SwapAppPositions(uint32_t appId1, uint32_t appId2) {
    std::vector<uint32_t>& appIds = shell->folders[shell->current_folder].app_ids;

    // Find positions of both apps
    int pos1 = -1, pos2 = -1;
    for (int i = 0; i < appIds.size(); i++)
    {
      if (appIds[i] == appId1)
        pos1 = i;
      if (appIds[i] == appId2)
        pos2 = i;
    }

    // Swap if both apps are found
    if (pos1 != -1 && pos2 != -1)
    {
      std::swap(appIds[pos1], appIds[pos2]);
      shell->SaveFolderVector(shell->current_folder);
    }
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

      // If this app is selected, show it brighter
      if (appId == shell->selected_app_id)
      {
        MatrixOS::LED::SetColor(origin + Point(x, y), appColor);
      }
      else
      {
        MatrixOS::LED::SetColor(origin + Point(x, y), appColor.Dim());
      }

      addedApps++;

      if (addedApps >= 8 * 7)
      {
        break;
      }
    }

    // Fill remaining spaces with black
    for (uint16_t i = addedApps; i < 8 * 7; i++)
    {
      uint8_t x = i % 8;
      uint8_t y = i / 8;
      MatrixOS::LED::SetColor(origin + Point(x, y), Color(0x000000));
    }

    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->State() == RELEASED)
    {
      uint8_t index = xy.y * 8 + xy.x;

      if (index < shell->folders[shell->current_folder].app_ids.size())
      {
        // Clicked on an app
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

        // Check if an app is already selected
        if (shell->selected_app_id != 0 && shell->selected_app_id != appId)
        {
          // Swap positions of selected app and clicked app
          SwapAppPositions(shell->selected_app_id, appId);
          shell->selected_app_id = 0; // Clear selection after swap
        }
        else if (shell->selected_app_id == appId)
        {
          // Deselect if clicking the same app
          shell->selected_app_id = 0;
        }
        else
        {
          // Select this app
          shell->selected_app_id = appId;
        }
        return true;
      }
      else
      {
        // Clicked on empty space - deselect current app
        if (shell->selected_app_id != 0)
        {
          shell->selected_app_id = 0;
          MLOGD("AppLauncherPickerEditMode", "Deselected app (clicked empty space)");
        }
        return true;
      }
    }
    else if (keyInfo->State() == HOLD)
    {
      uint8_t index = xy.y * 8 + xy.x;

      if (index < shell->folders[shell->current_folder].app_ids.size())
      {
        // Show app info on hold
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

        MatrixOS::UIUtility::TextScroll(application->name, application->color);
        return true;
      }
    }
    return false;
  }
};