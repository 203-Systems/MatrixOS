#include "MatrixOS.h"

class UILEDPartitionSelector : public UIComponent {
 public:
  uint8_t* selector;
  std::function<void(uint8_t)> callback;
  Color color;
  Color alternative_color;

  UILEDPartitionSelector(Color color, uint8_t* selector,  Color alternative_color = Color(0xFFFFFF), std::function<void(uint8_t)> callback = nullptr){
    this->selector = selector;
    this->callback = callback;
    this->color = color;
    this->alternative_color = alternative_color;
  }

  virtual Dimension GetSize() { return Dimension(Device::led_partitions.size(), 1); }
  virtual Color GetColor() { return color; }
  virtual Color GetAlternativeColor() { return alternative_color; }

  virtual bool Render(Point origin) {
    MatrixOS::LED::SetColor(origin, GetColor().ToLowBrightness(*selector == 0));
    for (uint8_t x = 1; x < Device::led_partitions.size(); x++) {
      MatrixOS::LED::SetColor(origin + Point(x, 0), GetAlternativeColor().ToLowBrightness(*selector == x));
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if(keyInfo->state == RELEASED && keyInfo->hold == false)
    {
      if(xy.x <= Device::led_partitions.size())
      {
        *selector = xy.x;
      }

      if(callback)
      {
        callback(*selector);
      }
    }
    else if (keyInfo->state == HOLD)
    {
      if(xy.x == 0)
      {
        MatrixOS::UIInterface::TextScroll("Global Brightness", GetColor());
      }
      else if(xy.x <= Device::led_partitions.size())
      {
        MatrixOS::UIInterface::TextScroll(Device::led_partitions[xy.x].name, GetAlternativeColor());
      }
    }
    
    return true;
  }
};