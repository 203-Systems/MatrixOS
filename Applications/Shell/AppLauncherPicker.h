
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

    virtual Dimension GetSize() { return Dimension(8, 7); }

    virtual bool Render(Point origin) {
        uint16_t added_apps = 0;
        for (uint8_t i = 0; i < shell->folders[shell->current_folder].app_ids.size(); i++)
        {
            uint32_t app_id = shell->folders[shell->current_folder].app_ids[i];
            auto application_it = shell->all_applications.find(app_id);
            if(application_it == shell->all_applications.end())
            {
                // Skip invalid app ID - should have been cleaned up on startup
                continue;
            }
            ApplicationEntry& application_entry = application_it->second;
            Application_Info* application_info = (application_entry.type == ApplicationType::Native) ?
                                                application_entry.native.info :
                                                &(application_entry.python.info->info);


            uint8_t x = added_apps % 8;
            uint8_t y = added_apps / 8;

            Color app_color = application_info->color;
            MatrixOS::LED::SetColor(origin + Point(x, y), app_color);
            added_apps++;

            if(added_apps >= 8 * 7)
            {
                break;
            }
        }
        return true;
    }

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
        if (keyInfo->State() == RELEASED || keyInfo->State() == HOLD) 
        {
            uint8_t index = xy.y * 8 + xy.x;
            if(index >= shell->folders[shell->current_folder].app_ids.size())
            {
                return false;
            }

            uint32_t app_id = shell->folders[shell->current_folder].app_ids[index];
            auto application_it = shell->all_applications.find(app_id);
            if(application_it == shell->all_applications.end())
            {
                // Skip invalid app ID - should have been cleaned up on startup
                return false;
            }
            ApplicationEntry& application_entry = application_it->second;
            Application_Info* application = (application_entry.type == ApplicationType::Native) ?
                                           application_entry.native.info :
                                           &(application_entry.python.info->info);


            if(keyInfo->State() == RELEASED)
            {
                MLOGD("Shell", "Launching App ID: %X", app_id);
                shell->LaunchAnimation(xy, application->color);

                if (application_entry.type == ApplicationType::Python) {
                    // Launch Python app with script path argument
                    vector<string> args = { application_entry.python.info->file_path };
                    MatrixOS::SYS::ExecuteAPP("203 Systems", "Python", args);
                } else {
                    // Launch native app normally
                    MatrixOS::SYS::ExecuteAPP(app_id);
                }
                return true;
            }
            else if(keyInfo->State() == HOLD)
            {
                MatrixOS::UIUtility::TextScroll(application->name, application->color);
                return true;
            }
        }
        return false;
    }
};