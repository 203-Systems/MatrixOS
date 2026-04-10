#include <cmath>
#include "MatrixOS.h"
#include "UI/UI.h"

class UIHueSelector : public UIComponent {
public:
  Dimension dimension;
  float begin;
  float step;
  UICallback<void(float)> callback;

  template <typename F>
  UIHueSelector(Dimension dimension, F&& callback, int16_t begin = 0, int16_t end = 360)
      : callback(static_cast<F&&>(callback)) {
    this->dimension = dimension;
    this->step = (float)(end - begin) / 360 / (dimension.Area() - 1);
    this->begin = std::fmod((float)begin / 360, 1.0);
  }

  virtual Dimension GetSize() {
    return dimension;
  }

  virtual bool Render(Point origin) {

    for (int8_t y = 0; y < dimension.y; y++)
    {
      int8_t uiY = dimension.y - y - 1;
      for (int8_t x = 0; x < dimension.x; x++)
      {
        float hue = std::fmod(begin + step * (y * dimension.x + x), 1.0);
        Color color = Color::HsvToRgb(hue, 1.0, 1.0).Gamma();
        MatrixOS::LED::SetColor(Point(x, uiY), color);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) {
    if (keypadInfo->state == KeypadState::Pressed)
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
  float* hue;
  float sBegin;
  float vBegin;
  float sStep;
  float vStep;
  UICallback<void(Color)> callback;

  template <typename F>
  UIShadeSelector(Dimension dimension, float* hue, F&& callback, Fract16 sBegin = 0, Fract16 sEnd = FRACT16_MAX,
                  Fract16 vBegin = 4096, Fract16 vEnd = FRACT16_MAX)
      : callback(static_cast<F&&>(callback)) {
    this->dimension = dimension;
    this->hue = hue;
    this->sStep = float(sEnd - sBegin) / (dimension.x - 1);
    this->vStep = float(vEnd - vBegin) / (dimension.y - 1);
    this->sBegin = float(sBegin);
    this->vBegin = float(vBegin);
  }

  virtual Dimension GetSize() {
    return dimension;
  }

  virtual bool Render(Point origin) {

    for (int8_t y = 0; y < dimension.y; y++)
    {
      int8_t uiY = dimension.y - y - 1;
      float v = vBegin + y * vStep;
      for (int8_t x = 0; x < dimension.x; x++)
      {
        float s = sBegin + x * sStep;
        Color color = Color::HsvToRgb(*hue, s, v).Gamma();
        MatrixOS::LED::SetColor(Point(x, uiY), color);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) {
    if (keypadInfo->state == KeypadState::Pressed)
    {
      int16_t uiY = dimension.y - xy.y - 1;
      float v = vBegin + uiY * vStep;
      float s = sBegin + xy.x * sStep;
      callback(Color::HsvToRgb(*hue, s, v).Gamma());
    }
    return true;
  }
};

namespace MatrixOS::UIUtility
{
bool ColorPicker(Color& color, bool shade) {
  float hue = 0;
  bool aborted = false;

  UI colorPicker("Color Picker", Color(0x000000));

  colorPicker.SetInputEventHandler([&](InputEvent* inputEvent) -> bool {
    if (inputEvent->id == InputId::FunctionKey())
    {
      if (inputEvent->keypad.state == KeypadState::Pressed)
      {
        aborted = true;
        colorPicker.Exit();
      }
      return true;
    }
    return false;
  });

  colorPicker.ClearUIComponents();

  // Phase 2 - Satuation + Value
  UIShadeSelector shadeSelector(Dimension(8, 8), &hue, [&](Color selectedColor) -> void {
    color = selectedColor;
    colorPicker.Exit();
  });
  shadeSelector.SetEnabled(false);
  colorPicker.AddUIComponent(shadeSelector, Point(0, 0));

  // Phase 1 - Hue selection
  UIHueSelector hueSelector(Dimension(8, 8), [&](float selectedHue) -> void {
    hue = selectedHue;

    if (shade == false)
    {
      color = Color::HsvToRgb(hue, 1.0f, 1.0f);
      colorPicker.Exit();
    }
    else
    {
      MatrixOS::LED::Fade();
      shadeSelector.SetEnabled(true);
      hueSelector.SetEnabled(false);
    }
  });
  colorPicker.AddUIComponent(hueSelector, Point(0, 0));

  colorPicker.Start();

  if (aborted)
  {
    return false;
  }

  // Return
  return true;
}
} // namespace MatrixOS::UIUtility
