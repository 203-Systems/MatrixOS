
#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "Shell.h"

class AppLauncherBar : public UIComponent {
  public:
    Shell* shell;
    AppLauncherBar(Shell* shell) {
        this->shell = shell;
    }

    virtual Dimension GetSize() { return Dimension(Shell::FOLDER_COUNT, 1); }

    virtual bool Render(Point origin) {
        uint8_t folder_count = 0;
        for (uint8_t i = 0; i < Shell::FOLDER_COUNT; i++)
        {
            Color color = shell->folder_colors[i];
            if(color == Color(0x000000)) // Folder not created
            {
                continue;
            }
            MatrixOS::LED::SetColor(origin + Point(folder_count, 0), color.DimIfNot(i == shell->current_folder));
            folder_count++;
        }
        return true;

    }

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
        if (keyInfo->State() == RELEASED || keyInfo->State() == HOLD) 
        {
            uint8_t folder_idx = xy.x;

            // Find the actual folder index
            uint8_t folder_real_idx = 0;
            for (uint8_t i = 0; i < Shell::FOLDER_COUNT; i++)
            {
                if(shell->folder_colors[i] != Color(0x000000)) // Folder not created
                {
                    if(folder_idx == 0)
                    {
                        folder_real_idx = i;
                        break;
                    }
                    folder_idx--;
                }
            }

            if(keyInfo->State() == RELEASED)
            {
                shell->current_folder = folder_real_idx;
                return true;
            }
            else if(keyInfo->State() == HOLD)
            {
                MatrixOS::UIUtility::TextScroll("Group " + std::to_string(folder_real_idx + 1), shell->folder_colors[folder_real_idx]);
                return true;
            }
        }
        return false;
    }
};