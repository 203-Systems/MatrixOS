#include "MatrixOS.h"
#include "UAD.h"
#include "ui/UI.h"

class UILayerControl : public UIComponent {
 public:
  string name;
  Color color;
  Dimension dimension;
  UAD* uad;
  UAD::LayerInfoType type;

  UILayerControl (string name, Color color, Dimension dimension, UAD* uad, UAD::LayerInfoType type) {
    this->name = name;
    this->color = color;
    this->dimension = dimension;
    this->uad = uad;
    this->type = type;
  }

  virtual string GetName() { return name; }
  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return dimension; }


  virtual bool Render(Point origin) {
    uint8_t layerCount = uad->layerCount;
    uint16_t bitmap = 0;
    if(type == UAD::LayerInfoType::ACTIVE)
    {
      bitmap = uad->layerEnabled;
    }
    else if(type == UAD::LayerInfoType::PASSTHROUGH)
    {
      bitmap = uad->layerPassthrough;
    }

    for (uint8_t x = 0; x < dimension.x; x++)
    {
      for (uint8_t y = 0; y < dimension.y; y++)
      {
        uint8_t id = y * dimension.x + x;
        Color layerColor = Color(0x101010);
        if(id < layerCount)
        {
          if(bitmap & (1 << id))
          {
            layerColor = color;
          }
          else
          {
            layerColor = color.Dim();
          }
        }
        MatrixOS::LED::SetColor(origin + Point(x, y), layerColor);
      }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    if (keyInfo->state == RELEASED && keyInfo->hold == false)
    { 
      uint8_t layer = xy.y * dimension.x + xy.x;
      if(layer == 0) return true; // Can't change root layer
      if(layer >= uad->layerCount) return true;
      uad->SetLayerState(layer, type, !uad->GetLayerState(layer, type));
    }
    else if (keyInfo->state == HOLD)
    {
        MatrixOS::UIInterface::TextScroll(GetName(), GetColor());
    }
    return true;
  }
};