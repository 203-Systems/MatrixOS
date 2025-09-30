#pragma once
#include "MatrixOS.h"
#include "UI/UI.h"
#include "Arpeggiator.h"

struct ArpDirVisual {
  uint8_t step[8]; // 8 steps to visualize the pattern, 0=no show, 1-4=first cycle, 5-8=second cycle
};

const ArpDirVisual arpDirVisuals[16] = {
  // 0 as no show, 1~4 means first repeat, 5~8 means second repeat
  /*ARP_UP*/              {{1, 2, 3, 4, 5, 6, 7, 8}},
  /*ARP_DOWN*/            {{4, 3, 2, 1, 8, 7, 6, 5}},
  /*ARP_UP_DOWN*/         {{1, 2, 3, 4, 3, 2, 5, 0}},
  /*ARP_DOWN_UP*/         {{4, 3, 2, 1, 2, 3, 8, 7}},
  /*ARP_UP_N_DOWN*/       {{1, 2, 3, 4, 4, 3, 2, 5}},
  /*ARP_DOWN_N_UP*/       {{4, 3, 2, 1, 1, 2, 3, 8}},
  /*ARP_RANDOM*/          {{0, 0, 0, 0, 0, 0, 0, 0}},
  /*ARP_PLAY_ORDER*/      {{0, 0, 0, 0, 0, 0, 0, 0}},
  /*ARP_CONVERGE*/        {{1, 4, 2, 3, 5, 8, 6, 7}},
  /*ARP_DIVERGE*/         {{2, 3, 1, 4, 6, 7, 5, 8}},
  /*ARP_CON_DIVERGE*/     {{1, 4, 2, 3, 2, 3, 1, 4}},
  /*ARP_DIV_CONVERGE*/    {{2, 3, 1, 4, 1, 4, 2, 3}},
  /*ARP_PINKY_UP*/        {{1, 4, 2, 4, 3, 4, 5, 8}},
  /*ARP_PINKY_UP_DOWN*/   {{1, 4, 2, 4, 3, 4, 2, 4}},
  /*ARP_THUMB_UP*/        {{1, 2, 1, 3, 1, 4, 5, 6}},
  /*ARP_THUMB_UP_DOWN*/   {{1, 2, 1, 3, 1, 4, 1, 3}},
};

class ArpDirVisualizer : public UIComponent {
 public:
  Color color;
  ArpDirection* direction;
  ArpDirection lastDirection;
  uint64_t lastUpdateTime = 0;
  uint8_t currentStep = 0;

  ArpDirVisualizer(ArpDirection* direction, Color color) {
    this->direction = direction;
    this->lastDirection = *direction;
    this->color = color;
  }

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(8, 4); }

  bool IsEnabled() {
     if (enableFunc) {
      enabled = (*enableFunc)();
    }
    if(!enabled)
    {
      currentStep = 0;
      lastUpdateTime = 0;
    }
    return enabled;
  }

  virtual bool Render(Point origin) {
    // For other modes, use the animated pattern from arpDirVisuals
    uint64_t currentTime = MatrixOS::SYS::Millis();
    
    // Reset animation if direction changed or not inited
    if (*direction != lastDirection || lastUpdateTime == 0) {
      currentStep = 0;
      lastUpdateTime = currentTime;
      lastDirection = *direction;
    }

    if(*direction == ARP_RANDOM)
    {
      // R
      MatrixOS::LED::SetColor(origin + Point(0, 0), color);
      MatrixOS::LED::SetColor(origin + Point(0, 1), color);
      MatrixOS::LED::SetColor(origin + Point(0, 2), color);
      MatrixOS::LED::SetColor(origin + Point(0, 3), color);
      MatrixOS::LED::SetColor(origin + Point(1, 0), color);
      MatrixOS::LED::SetColor(origin + Point(1, 2), color);
      MatrixOS::LED::SetColor(origin + Point(2, 0), color);
      MatrixOS::LED::SetColor(origin + Point(2, 1), color);
      MatrixOS::LED::SetColor(origin + Point(2, 3), color);

      // n
      MatrixOS::LED::SetColor(origin + Point(3, 1), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(3, 2), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(3, 3), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(4, 1), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(5, 2), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(5, 3), Color(0xFFFFFF));

      // d
      MatrixOS::LED::SetColor(origin + Point(6, 2), color);
      MatrixOS::LED::SetColor(origin + Point(6, 3), color);
      MatrixOS::LED::SetColor(origin + Point(7, 0), color);
      MatrixOS::LED::SetColor(origin + Point(7, 1), color);
      MatrixOS::LED::SetColor(origin + Point(7, 2), color);
      MatrixOS::LED::SetColor(origin + Point(7, 3), color);
    }
    else if(*direction == ARP_PLAY_ORDER)
    {
      // O
      MatrixOS::LED::SetColor(origin + Point(0, 0), color);
      MatrixOS::LED::SetColor(origin + Point(0, 1), color);
      MatrixOS::LED::SetColor(origin + Point(0, 2), color);
      MatrixOS::LED::SetColor(origin + Point(0, 3), color);
      MatrixOS::LED::SetColor(origin + Point(1, 0), color);
      MatrixOS::LED::SetColor(origin + Point(1, 3), color);
      MatrixOS::LED::SetColor(origin + Point(2, 0), color);
      MatrixOS::LED::SetColor(origin + Point(2, 1), color);
      MatrixOS::LED::SetColor(origin + Point(2, 2), color);
      MatrixOS::LED::SetColor(origin + Point(2, 3), color);

      // R
      MatrixOS::LED::SetColor(origin + Point(3, 0), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(3, 1), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(3, 2), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(3, 3), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(4, 0), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(4, 2), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(5, 0), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(5, 1), Color(0xFFFFFF));
      MatrixOS::LED::SetColor(origin + Point(5, 3), Color(0xFFFFFF));

      // d
      MatrixOS::LED::SetColor(origin + Point(6, 2), color);
      MatrixOS::LED::SetColor(origin + Point(6, 3), color);
      MatrixOS::LED::SetColor(origin + Point(7, 0), color);
      MatrixOS::LED::SetColor(origin + Point(7, 1), color);
      MatrixOS::LED::SetColor(origin + Point(7, 2), color);
      MatrixOS::LED::SetColor(origin + Point(7, 3), color);
    }
    else
    {
      // Update animation step every 300ms
      if (currentTime - lastUpdateTime > 300) {
        currentStep = (currentStep + 1) % 9;
        lastUpdateTime = currentTime;
      }

      const ArpDirVisual& visual = arpDirVisuals[*direction];

      for (uint8_t i = 0; i < 8; i++) {
        uint8_t stepValue = visual.step[i];

        if (stepValue == 0) {
          // No show - skip
          continue;
        }

        uint8_t y = stepValue; // 1-based y coordinate
        Point xy;
        Color ledColor;

        if (y <= 4) {
          // Normal range: render at Point(i, 4-y)
          xy = origin + Point(i, 4 - y);
          if (i < currentStep) {
            ledColor = color; // Current step - bright
          } else {
            ledColor = color.Dim(); // Dim
          }
        } else {
          // y > 4: render at Point(i, 8-y) with white color
          xy = origin + Point(i, 8 - y);
          if (i < currentStep) {
            ledColor = Color(0xFFFFFF); // Current step - bright white
          } else {
            ledColor = Color(0xFFFFFF).Dim(); // Dim white
          }
        }

        MatrixOS::LED::SetColor(xy, ledColor);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    // No key interaction for visualizer
    if(keyInfo->State() == HOLD)
    {
      MatrixOS::UIUtility::TextScroll(arpDirectionNames[*direction], color);
    }
    return false;
  }
};