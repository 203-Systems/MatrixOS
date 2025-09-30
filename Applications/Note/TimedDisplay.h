#pragma once
#include "MatrixOS.h"
#include "UI/UI.h"


class TimedDisplay : public UIComponent {
 public:
  uint32_t lastEnabledTime = 0;
  uint32_t enableLength;
  Dimension dimension;
  std::unique_ptr<std::function<void(Point)>> renderFunc;

  TimedDisplay(uint32_t enableLength = 500) {
    this->lastEnabledTime = 0;
    this->enableLength = enableLength;
    this->renderFunc = nullptr;
  }

  void SetRenderFunc(std::function<void(Point)> renderFunc) {
    this->renderFunc = std::make_unique<std::function<void(Point)>>(renderFunc);
  }

  void SetDimension(Dimension dimension) {
    this->dimension = dimension;
  }

  virtual Dimension GetSize() { return dimension; }

  bool IsEnabled() {
    uint32_t currentTime = (uint32_t)MatrixOS::SYS::Millis();
    if (enableFunc) {
      enabled = (*enableFunc)();
    }

    // If already timed out, force disabled
    if(enabled && lastEnabledTime == UINT32_MAX)
    {
      enabled = false;
      return enabled;
    }

    // Detect rising edge: lastEnabledTime == 0 means was disabled, now enabled
    if(enabled && lastEnabledTime == 0)
    {
      lastEnabledTime = currentTime;
      return enabled;
    }

    // If disabled externally, reset timer
    if(!enabled)
    {
      lastEnabledTime = 0;
      return enabled;
    }

    // Check timeout - only if we have a valid start time
    if(currentTime - lastEnabledTime > enableLength)
    {
      lastEnabledTime = UINT32_MAX;  // Mark as timed out (not 0 to avoid re-triggering)
      enabled = false;  // Override to disabled due to timeout
    }

    return enabled;
  }

  void Disable()
  {
    lastEnabledTime = UINT32_MAX;
    enabled = false;
  }

  virtual bool Render(Point origin) {
    for(int8_t y = 0; y < dimension.y; y++)
    {
      for(int8_t x = 0; x < dimension.x; x++)
      {
        MatrixOS::LED::SetColor(origin + Point(x, y), Color(0));
      }
    }

    if (renderFunc) 
    {
      (*renderFunc)(origin);
    }

    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if(keyInfo->State() == PRESSED)
    {
      Disable();
    }
    return true;// Block keypress
  }
};