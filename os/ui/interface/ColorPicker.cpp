#include <cmath>
#include "MatrixOS.h"
#include "ui/UI.h"

class UIHueSelector : public UIComponent {
 public:
  Dimension dimension;
  float begin;
  float step;
  std::function<void(float)> callback;

  UIHueSelector(Dimension dimension, std::function<void(float)> callback, int16_t begin = 0, int16_t end = 360) {
    this->dimension = dimension;
    this->step = (float)(end - begin) / 360 / (dimension.Area() - 1);
    this->begin = std::fmod((float)begin / 360, 1.0);
    this->callback = callback;
  }

  virtual Dimension GetSize() { return dimension; }

  virtual bool Render(Point origin) {

    for (int8_t y = 0; y < dimension.y; y++)
    {
      int8_t ui_y = dimension.y - y - 1;
      for (int8_t x = 0; x < dimension.x; x++)
      {
        float hue = std::fmod(begin + step * (y * dimension.x + x), 1.0);
        Color color = Color::HsvToRgb(hue, 1.0, 1.0);
        MatrixOS::LED::SetColor(Point(x, ui_y), color);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == PRESSED)
    {
      uint8_t index = (dimension.y - xy.y - 1) * dimension.x + xy.x;
      float hue = std::fmod(begin + step * index, 1.0);
      callback(hue);
    }
    return true;
  }
};

class UIShadeSelector : public UIComponent {
 public:
  Dimension dimension;
  float hue;
  float sBegin;
  float vBegin;
  float sStep;
  float vStep;
  std::function<void(Color)> callback;

  UIShadeSelector(Dimension dimension, float hue, std::function<void(Color)> callback, Fract16 sBegin = 0, Fract16 sEnd = FRACT16_MAX, Fract16 vBegin = 0,
                  Fract16 vEnd = FRACT16_MAX) {
    this->dimension = dimension;
    this->hue = hue;
    this->sStep = float(sEnd - sBegin) / (dimension.x - 1);
    this->vStep = float(vEnd - vBegin) / (dimension.y - 1);
    this->sBegin = float(sBegin);
    this->vBegin = float(vBegin);
    this->callback = callback;
  }

  virtual Dimension GetSize() { return dimension; }

  virtual bool Render(Point origin) {

    for (int8_t y = 0; y < dimension.y; y++)
    {
      int8_t ui_y = dimension.y - y - 1;
      float v = vBegin + y * vStep;
      for (int8_t x = 0; x < dimension.x; x++)
      {
        float s = sBegin + x * sStep;
        Color color = Color::HsvToRgb(hue, s, v);
        MatrixOS::LED::SetColor(Point(x, ui_y), color);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == PRESSED)
    {
        int16_t ui_y = dimension.y - xy.y - 1;
        float v = vBegin + ui_y * vStep;
        float s = sBegin + xy.x * sStep;
        callback(Color::HsvToRgb(hue, s, v));
    }
    return true;
  }
};

namespace MatrixOS::UIInterface
{
  bool ColorPicker(Color& color) {
    float hue = 0;
    bool aborted = false;

    // Setup
    UI colorPicker("Color Picker");

    colorPicker.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
      if (keyEvent->id == FUNCTION_KEY)
      {
        if (keyEvent->info.state == PRESSED)
        {
          aborted = true;
          colorPicker.Exit();
        }
        return true;
      }
      return false;
    });

    // Phase 1 - Hue selection
    UIHueSelector hueSelector(Dimension(8, 8), [&](float selected_hue) -> void {
      hue = selected_hue;
      colorPicker.Exit();
    });
    colorPicker.AddUIComponent(hueSelector, Point(0, 0));
    colorPicker.Start();

    if(aborted)
    { return false; }

    colorPicker.ClearUIComponents();

    // Phase 2 - Satuation + Value
    UIShadeSelector shadeSelector(Dimension(8, 8), hue, [&](Color selectedColor) -> void {
      color = selectedColor;
      colorPicker.Exit();
    });
    colorPicker.AddUIComponent(shadeSelector, Point(0, 0));
    colorPicker.Start();

    if (aborted)
    { return false; }

    // Return
    return true;
  }
}
