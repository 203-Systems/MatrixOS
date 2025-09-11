
#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
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
            auto application_it = applications.find(app_id);
            if(application_it == applications.end())
            {
                MLOGE("Shell", "App ID %X not found in application list", app_id);
                continue;
            }
            Application_Info* application_info = application_it->second;


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
        if (keyInfo->state == RELEASED || keyInfo->state == HOLD) 
        {
            uint8_t index = xy.y * 8 + xy.x;
            if(index >= shell->folders[shell->current_folder].app_ids.size())
            {
                return false;
            }

            uint32_t app_id = shell->folders[shell->current_folder].app_ids[index];
            auto application_it = applications.find(app_id);
            if(application_it == applications.end())
            {
                MLOGE("Shell", "App ID %X not found in application list", app_id);
                return false;
            }
            Application_Info* application = application_it->second;


            if(keyInfo->state == RELEASED)
            {
                MLOGD("Shell", "Launching App ID: %d", app_id);
                shell->LaunchAnimation(xy, application->color);
                MatrixOS::SYS::ExecuteAPP(app_id);
                return true;
            }
            else if(keyInfo->state == HOLD)
            {
                MatrixOS::UIUtility::TextScroll(application->name, application->color);
                return true;
            }
        }
        return false;
    }
};