#include "MatrixOS.h"

class UITwoToneSelector : public UIComponent {
 public:
  Dimension dimension;
  uint16_t count;
  Color color1;
  Color color2;
  uint8_t* value;
  uint8_t threshold;
  uint8_t* map;
  std::function<void(uint8_t)> callback;

  UITwoToneSelector(Dimension dimension, uint16_t count, Color color1, Color color2, uint8_t* value, uint8_t threshold,
                    uint8_t* map, std::function<void(uint8_t)> callback) {
    this->dimension = dimension;
    this->count = count;
    this->color1 = color1;
    this->color2 = color2;
    this->value = value;
    this->threshold = threshold;
    this->map = map;
    this->callback = callback;
  }

  virtual Dimension GetSize() { return dimension; }

  virtual bool Render(Point origin) {
    for (uint8_t y = 0; y < dimension.y; y++)
    {
      for (uint8_t x = 0; x < dimension.x; x++)
      {
        uint16_t index = y * dimension.x + x;
        Point target_coord = origin + Point(x, y);
        Color target_color = Color(0);
        if (index < count)
        {
          target_color = map[y * dimension.x + x] > threshold ? color2 : color1;
          target_color = target_color.ToLowBrightness(*value >= map[y * dimension.x + x]);
        }
        MatrixOS::LED::SetColor(target_coord, target_color);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    uint16_t index = xy.y * dimension.x + xy.x;
    if (index < count)
    {
      uint8_t value = map[xy.y * dimension.x + xy.x];
      callback(value);
    }
    return true;
  }
};