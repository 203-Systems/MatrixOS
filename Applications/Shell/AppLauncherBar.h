
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

  virtual Dimension GetSize() {
    return Dimension(Shell::folderCount, 1);
  }

  virtual bool Render(Point origin) {
    uint8_t folderCount = 0;
    for (uint8_t i = 0; i < Shell::folderCount; i++)
    {
      Color color = shell->folder_colors[i];
      if (color == Color(0x000000)) // Folder not created
      {
        continue;
      }
      MatrixOS::LED::SetColor(origin + Point(folderCount, 0), color.DimIfNot(i == shell->current_folder));
      folderCount++;
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->State() == RELEASED || keyInfo->State() == HOLD)
    {
      uint8_t folderIdx = xy.x;

      // Find the actual folder index
      uint8_t folderRealIdx = 0;
      for (uint8_t i = 0; i < Shell::folderCount; i++)
      {
        if (shell->folder_colors[i] != Color(0x000000)) // Folder not created
        {
          if (folderIdx == 0)
          {
            folderRealIdx = i;
            break;
          }
          folderIdx--;
        }
      }

      if (keyInfo->State() == RELEASED)
      {
        shell->current_folder = folderRealIdx;
        return true;
      }
      else if (keyInfo->State() == HOLD)
      {
        MatrixOS::UIUtility::TextScroll("Group " + std::to_string(folderRealIdx + 1), shell->folder_colors[folderRealIdx]);
        return true;
      }
    }
    return false;
  }
};