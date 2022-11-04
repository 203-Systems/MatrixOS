#include "MatrixOS.h"

class UISelector : public UIComponent {
 public:
  Color color;
  string name;
  Dimension dimension;
  uint16_t* value;
  uint16_t count;
  std::function<void(uint16_t)> callback;

  UISelector(Dimension dimension, string name, Color color, uint16_t count, uint16_t* value, std::function<void(uint16_t)> callback = nullptr) {
    this->dimension = dimension;
    this->name = name;
    this->color = color;
    this->value = value;
    this->count = count;
    this->callback = callback;
  }

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return dimension; }

  void SetColor(Color color) { this->color = color; }

  virtual bool Render(Point origin) {
    for (uint16_t item = 0; item < dimension.Area(); item++)
    {
      // Maybe allow different direction
      Point xy = origin + Point(item % dimension.x, item / dimension.x);
      if (item > count)
      { MatrixOS::LED::SetColor(xy, Color(0)); }
      else
      { MatrixOS::LED::SetColor(xy, color.ToLowBrightness(item == *value)); }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == HOLD)
    {
      MatrixOS::UIInterface::TextScroll(name, GetColor());
      return true;
    }
    uint16_t id = xy.x + xy.y * dimension.x;
    if (id > count)
    { return false; }
    if (keyInfo->state == RELEASED)
    {
      *value = id;
      if (callback)
        callback(id);
    }
    return true;
  }
};