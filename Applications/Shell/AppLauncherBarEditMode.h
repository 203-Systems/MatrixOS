#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "Shell.h"


class AppLauncherBarEditMode : public UIComponent {
  public:
    Shell* shell;
    uint32_t start_time;
    
    AppLauncherBarEditMode(Shell* shell) {
        this->shell = shell;
        this->start_time = MatrixOS::SYS::Millis();
    }

    virtual Dimension GetSize() { return Dimension(7, 1); }

    virtual bool Render(Point origin) {
        // Render existing folders
        for (uint8_t i = 0; i < Shell::FOLDER_COUNT; i++)
        {
            Color color = shell->folder_colors[i];
            if(color != Color(0x000000)) // Folder exists
            {
                // Show folder color, dimmed if not the current folder
                MatrixOS::LED::SetColor(origin + Point(i, 0), color.DimIfNot(i == shell->current_folder));
            }
            else
            {
                // Empty slot - show as dark gray
                MatrixOS::LED::SetColor(origin + Point(i, 0), ColorEffects::ColorBreathLowBound(Color(0xFFFFFF).Dim(), 64, 1000, start_time));
            }
        }

        // Hidden App page
        MatrixOS::LED::SetColor(origin + Point(6, 0), Color(0xFF00FF).DimIfNot(shell->current_folder == Shell::FOLDER_HIDDEN));
        
        return true;
    }

    void FolderSettings(uint8_t folder_idx)
    {
        UI folderSettingsUI("Folder Settings", Color(0xFFFFFF), true);

        // Color picker
        UIButton colorPickerBtn;
        colorPickerBtn.SetName("Group Color");
        colorPickerBtn.SetSize(Dimension(6, 3));
        colorPickerBtn.SetColorFunc([&]() -> Color { return shell->folder_colors[folder_idx]; });
        colorPickerBtn.OnPress([&]() -> void {
            Color new_color = shell->folder_colors[folder_idx];
            if (MatrixOS::UIUtility::ColorPicker(new_color))
            {
                shell->EnableFolder(folder_idx, new_color);
            }
        });

        folderSettingsUI.AddUIComponent(colorPickerBtn, Point(1, 1));

        // Delete folder
        // Ask for confirm
        UIButton deleteFolderBtn;
        deleteFolderBtn.SetName("Remove Group");
        deleteFolderBtn.SetColor(Color(0xFF0000));
        deleteFolderBtn.SetSize(Dimension(4, 1));
        deleteFolderBtn.OnPress([&]() -> void {
            UI confirmDeleteUI("Confirm Group Deletion", Color(0xFF0000));

            confirmDeleteUI.SetPreRenderFunc([]() -> void {
                // D
                MatrixOS::LED::SetColor(Point(0, 0), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(0, 1), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(0, 2), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(0, 3), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(1, 0), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(1, 3), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(2, 1), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(2, 2), Color(0xFF0000));
                // E
                MatrixOS::LED::SetColor(Point(3, 0), Color(0xFFFFFF));
                MatrixOS::LED::SetColor(Point(3, 1), Color(0xFFFFFF));
                MatrixOS::LED::SetColor(Point(3, 2), Color(0xFFFFFF));
                MatrixOS::LED::SetColor(Point(3, 3), Color(0xFFFFFF));
                MatrixOS::LED::SetColor(Point(4, 0), Color(0xFFFFFF));
                MatrixOS::LED::SetColor(Point(4, 1), Color(0xFFFFFF));
                MatrixOS::LED::SetColor(Point(4, 3), Color(0xFFFFFF));
                MatrixOS::LED::SetColor(Point(5, 0), Color(0xFFFFFF));
                MatrixOS::LED::SetColor(Point(5, 3), Color(0xFFFFFF));

                // L
                MatrixOS::LED::SetColor(Point(6, 0), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(6, 1), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(6, 2), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(6, 3), Color(0xFF0000));
                MatrixOS::LED::SetColor(Point(7, 3), Color(0xFF0000));
            });

            UIButton cancelDeleteBtn;
            cancelDeleteBtn.SetName("Cancel");
            cancelDeleteBtn.SetColor(Color(0xFFFFFF));
            cancelDeleteBtn.SetSize(Dimension(2, 2));
            cancelDeleteBtn.OnPress([&]() -> void { confirmDeleteUI.Exit(); });
            confirmDeleteUI.AddUIComponent(cancelDeleteBtn, Point(1, 5));

            UIButton confirmDeleteBtn;
            confirmDeleteBtn.SetName("Confirm");
            confirmDeleteBtn.SetColor(Color(0xFF0000));
            confirmDeleteBtn.SetSize(Dimension(2, 2));
            confirmDeleteBtn.OnPress([&]() -> void {
                shell->DisableFolder(folder_idx);
                confirmDeleteUI.Exit();
                folderSettingsUI.Exit();
            });
            confirmDeleteUI.AddUIComponent(confirmDeleteBtn, Point(5, 5));

            confirmDeleteUI.Start();
        });
        folderSettingsUI.AddUIComponent(deleteFolderBtn, Point(2, 6));

        folderSettingsUI.Start();
    }

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
        if (keyInfo->State() == RELEASED) 
        {   
            if (xy.x < 7) {
                uint8_t folder_idx = xy.x == 6 ? Shell::FOLDER_HIDDEN : xy.x;
                
                // Check if folder is active (exists)
                if (folder_idx != Shell::FOLDER_HIDDEN && shell->folder_colors[folder_idx] == Color(0x000000)) {
                    // Folder doesn't exist - enable it
                    Color new_color = Color(0x000000);
                    if (!MatrixOS::UIUtility::ColorPicker(new_color))
                    {
                        return true; // User cancelled color picker
                    }
                    
                    shell->EnableFolder(folder_idx, new_color);
                }
                // Now handle app movement or folder switching
                else if (shell->selected_app_id != 0) {
                    // App is selected - move it to this folder
                    shell->MoveAppToFolder(shell->selected_app_id, folder_idx);
                    shell->selected_app_id = 0; // Clear selection
                    MLOGD("AppLauncherBarEditMode", "Moved app to folder %d", folder_idx);
                // No app selected - just switch to the folder
                } else {
                    shell->current_folder = folder_idx;
                    MLOGD("AppLauncherBarEditMode", "Switched to folder %d", folder_idx);
                }
                return true;
            }
        }
        else if(keyInfo->State() == HOLD && xy.x < 6)
        {
            if (shell->folder_colors[xy.x] == Color(0x000000)) {
                // Folder doesn't exist
                MatrixOS::UIUtility::TextScroll("Active Group " + std::to_string(xy.x + 1), Color(0xFFFFFF));
            }
            else
            {
                // Folder exists
                // Open folder settings on hold
                FolderSettings(xy.x);
            }
            return true;
        }
        else if(keyInfo->State() == HOLD && xy.x == 6)
        {
            // Show hidden folder info on hold
            MatrixOS::UIUtility::TextScroll("Hidden Apps", Color(0xFF00FF));
            return true;
        }
        return false;
    }
};