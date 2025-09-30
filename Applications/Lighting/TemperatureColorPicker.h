#include <cmath>
#include "MatrixOS.h"
#include "UI/UI.h"

#define DEFAULT_CENTER_TEMP 6500
class UITemperatureColorSelector : public UIComponent {
 public:
  Dimension dimension;
  uint16_t begin;
  uint16_t step;
  std::function<void(Color)> callback;

  UITemperatureColorSelector(Dimension dimension, std::function<void(Color)> callback, uint16_t begin = 300, uint16_t end = 6700) {
    this->dimension = dimension;
    this->step = (end - begin) / dimension.Area();
    this->begin = begin;
    this->callback = callback;
  }

  virtual Dimension GetSize() { return dimension; }

  Color TemperatureToRGB(uint16_t temperature)
  {
  float red, green, blue = 0;
  float temp = temperature * 0.01;

   if (temp <= 66) {
    red = 255;
  } else {
    red = temp - 60;
    red = 329.698727466 * std::pow(red, -0.1332047592);
    if (red < 0) {
      red = 0;
    }
    if (red > 255) {
      red = 255;
    }
  }

  if (temp <= 66) {
    green = temp;
    green = 99.4708025861 * std::log(green) - 161.1195681661;
    if (green < 0) {
      green = 0;
    }
    if (green > 255) {
      green = 255;
    }
  } else {
    green = temp - 60;
    green = 288.1221695283 * std::pow(green, -0.0755148492);
    if (green < 0) {
      green = 0;
    }
    if (green > 255) {
      green = 255;
    }
  }

  if (temp >= 66) {
    blue = 255;
  } else {
    if (temp <= 19) {
      blue = 0;
    } else {
      blue = temp - 10;
      blue = 138.5177312231 * std::log(blue) - 305.0447927307;
      if (blue < 0) {
        blue = 0;
      }
      if (blue > 255) {
        blue = 255;
      }
    }
  }


  // if (t <= 66)
  // {
  //   red = 255;
  //   green = t - 2;
  //   green = -155.25485562709179 - 0.44596950469579133 * green + 104.49216199393888 * log(green);
  //   blue = 0;
  //   if (t > 20)
  //   {
  //     blue = t - 10;
  //     blue = -254.76935184120902 + 0.8274096064007395 * blue + 115.67994401066147 * log(blue);
  //   }
  // }
  // else
  // {
  //   red = t - 55.0;
  //   red = 351.97690566805693 + 0.114206453784165 * red - 40.25366309332127 * log(red);
  //   green = t - 50.0;
  //   green = 325.4494125711974 + 0.07943456536662342 * green - 28.0852963507957 * log(green);
  //   blue = 255;
  // }

  // if (t <= 66)
  // {
  //   red = 255;
  //   green = (99.4708025861 * log(t)) - 161.1195681661;
  //   if (t > 19)
  //   {
  //     blue = (138.5177312231 * log(t - 10)) - 305.0447927307;
  //   }
  //   else blue = 0;
  // }
  // else
  // {
  //   red   = 329.698727446  * pow(t - 60, -0.1332047592);
  //   green = 288.1221695283 * pow(t - 60, -0.0755148492);
  //   blue  = 255;
  // }

  // Cap Red green and blue between - and 255
  
  // red = std::max(0.0f, std::min(255.0f, red));
  // green = std::max(0.0f, std::min(255.0f, green));
  // blue = std::max(0.0f, std::min(255.0f, blue));

  return Color(red, green, blue);
  }

  virtual bool Render(Point origin) {

    for (int8_t y = 0; y < dimension.y; y++)
    {
      int8_t ui_y = dimension.y - y - 1;
      for (int8_t x = 0; x < dimension.x; x++)
      {
        uint8_t index = y * dimension.x + x;
        uint16_t temp = begin + step * index;
        Color color = TemperatureToRGB(temp);
        MatrixOS::LED::SetColor(Point(x, ui_y), color);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->State() == RELEASED)
    {
      uint8_t index = (dimension.y - xy.y - 1) * dimension.x + xy.x;
      uint16_t temp = begin + step * index;
      Color color = TemperatureToRGB(temp);
      callback(color);
    }
    else if(keyInfo->State() == HOLD)
    {
      uint8_t index = (dimension.y - xy.y - 1) * dimension.x + xy.x;
      uint16_t temp = begin + step * index;
      string temp_str = std::to_string(temp) + "K";
      Color color = TemperatureToRGB(temp);
      MatrixOS::UIUtility::TextScroll(temp_str, color);
    }
    return true;
  }
};

namespace MatrixOS::UIUtility
{
  inline bool TemperatureColorPicker(Color& color) {
    bool aborted = false;

    // Setup
    // int8_t old_layer = MatrixOS::LED::CurrentLayer();

    // int8_t new_layer = MatrixOS::LED::CreateLayer();
    UI colorPicker("Temperature Color Picker", Color(0x000000), false);

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

    UITemperatureColorSelector tempColorSelector(Dimension(8, 8), [&](Color new_color) -> void {
      color = new_color;
      colorPicker.Exit();
    });
    colorPicker.AddUIComponent(tempColorSelector, Point(0, 0));

    colorPicker.Start();


    if(aborted)
    { 
      return false; 
    }

    // Return
    return true;
  }
}
