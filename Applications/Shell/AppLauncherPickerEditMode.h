#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
#include "Shell.h"

class AppLauncherPickerEditMode : public UIComponent {
  public:
    Shell* shell;
    
    AppLauncherPickerEditMode(Shell* shell) {
        this->shell = shell;
    }
    
    void SwapAppPositions(uint32_t app_id1, uint32_t app_id2) {
        std::vector<uint32_t>& app_ids = shell->folders[shell->current_folder].app_ids;
        
        // Find positions of both apps
        int pos1 = -1, pos2 = -1;
        for (int i = 0; i < app_ids.size(); i++) {
            if (app_ids[i] == app_id1) pos1 = i;
            if (app_ids[i] == app_id2) pos2 = i;
        }
        
        // Swap if both apps are found
        if (pos1 != -1 && pos2 != -1) {
            std::swap(app_ids[pos1], app_ids[pos2]);
            shell->SaveFolderVector(shell->current_folder);
        }
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
                // Skip invalid app ID - should have been cleaned up on startup
                continue;
            }
            Application_Info* application_info = application_it->second;

            uint8_t x = added_apps % 8;
            uint8_t y = added_apps / 8;

            Color app_color = application_info->color;
            
            // If this app is selected, show it brighter
            if (app_id == shell->selected_app_id) {
                MatrixOS::LED::SetColor(origin + Point(x, y), app_color);
            }
            else
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), app_color.Dim());
            }
            
            added_apps++;

            if(added_apps >= 8 * 7)
            {
                break;
            }
        }
        
        // Fill remaining spaces with black
        for (uint16_t i = added_apps; i < 8 * 7; i++) {
            uint8_t x = i % 8;
            uint8_t y = i / 8;
            MatrixOS::LED::SetColor(origin + Point(x, y), Color(0x000000));
        }
        
        return true;
    }

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
        if (keyInfo->state == RELEASED) 
        {
            uint8_t index = xy.y * 8 + xy.x;
            
            if(index < shell->folders[shell->current_folder].app_ids.size())
            {
                // Clicked on an app
                uint32_t app_id = shell->folders[shell->current_folder].app_ids[index];
                auto application_it = applications.find(app_id);
                if(application_it == applications.end())
                {
                    // Skip invalid app ID - should have been cleaned up on startup
                    return false;
                }
                Application_Info* application = application_it->second;
                
                // Check if an app is already selected
                if (shell->selected_app_id != 0 && shell->selected_app_id != app_id) {
                    // Swap positions of selected app and clicked app
                    SwapAppPositions(shell->selected_app_id, app_id);
                    shell->selected_app_id = 0; // Clear selection after swap
                } else if (shell->selected_app_id == app_id) {
                    // Deselect if clicking the same app
                    shell->selected_app_id = 0;
                } else {
                    // Select this app
                    shell->selected_app_id = app_id;
                }
                return true;
            }
            else
            {
                // Clicked on empty space - deselect current app
                if (shell->selected_app_id != 0) {
                    shell->selected_app_id = 0;
                    MLOGD("AppLauncherPickerEditMode", "Deselected app (clicked empty space)");
                }
                return true;
            }
        }
        else if(keyInfo->state == HOLD)
        {
            uint8_t index = xy.y * 8 + xy.x;
            
            if(index < shell->folders[shell->current_folder].app_ids.size())
            {
                // Show app info on hold
                uint32_t app_id = shell->folders[shell->current_folder].app_ids[index];
                auto application_it = applications.find(app_id);
                if(application_it == applications.end())
                {
                    // Skip invalid app ID - should have been cleaned up on startup
                    return false;
                }
                Application_Info* application = application_it->second;
                
                MatrixOS::UIUtility::TextScroll(application->name, application->color);
                return true;
            }
        }
        return false;
    }
};