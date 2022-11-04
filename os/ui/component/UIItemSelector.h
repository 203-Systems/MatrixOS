#include "MatrixOS.h"

template <class T>
class UIItemSelector : public UIComponent {
 public:
  Color color;
  T* output;
  T* items;
  string* names;
  Dimension dimension;
  uint16_t count;

  UIItemSelector(Dimension dimension, Color color, T* output, uint16_t count, T* items, string* names = nullptr) {
    this->dimension = dimension;
    this->color = color;
    this->output = output;
    this->count = count;
    this->items = items;
    this->names = names;
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
      { MatrixOS::LED::SetColor(xy, color.ToLowBrightness(items[item] == *output)); }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    uint16_t id = xy.x + xy.y * dimension.x;
    if (id > count)
      return false;
    if (names == nullptr)
    {
      if (keyInfo->state == PRESSED)
      { *output = items[id]; }
    }
    else
    {
      if (keyInfo->state == RELEASED)
      { *output = items[id]; }
      else if (keyInfo->state == HOLD)
      { MatrixOS::UIInterface::TextScroll(names[id], GetColor()); }
    }
    return true;
  }
};