#include "MatrixOS.h"

class ScaleVisualizer : public UIComponent {
 public:
  uint8_t* rootKey;
  uint16_t* scale;
  Color color;
  Color rootColor;

  ScaleVisualizer(uint8_t* rootKey, uint16_t* scale, Color color = Color(0x00FFFF), Color rootColor = Color(0x0000FF)) {
    this->rootKey = rootKey;
    this->scale = scale;
    this->color = color;
    this->rootColor = rootColor;
  }

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(2, 8); }

  virtual bool Render(Point origin) {
    uint16_t c_aligned_scale_map = ((*scale << *rootKey) + ((*scale & 0xFFF) >> (12 - *rootKey % 12))) & 0xFFF; //Rootkey should always < 12, might add an assert later
    for (uint8_t note = 0; note < 12; note++)
    {
      Point xy = origin + ((note > 6) ? Point((note + 1) / 2, note % 2) : Point((note + 2) / 2, (note + 1) % 2));
      if (note == *rootKey)
      { MatrixOS::LED::SetColor(xy, rootColor); }
      else if (bitRead(c_aligned_scale_map, note))
      { MatrixOS::LED::SetColor(xy, color); }
      else
      { MatrixOS::LED::SetColor(xy, color.ToLowBrightness()); }
    }
    return true;
  }

virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    return false;
  }
};