#pragma once
#include "MatrixOS.h"
#include "UI/UI.h"
#include "Arpeggiator.h"

class RhythmVisualizer : public UIComponent {
 public:
  Color color;
  Color rootColor;
  Arpeggiator* arp;

  RhythmVisualizer(Color color, Color rootColor, Arpeggiator* arp){
    this->color = color;
    this->rootColor = rootColor;
    this->arp = arp;
  }

  virtual Color GetColor() { return color; }
  virtual Dimension GetSize() { return Dimension(8, 4); }

  virtual bool Render(Point origin) {
    for(uint8_t i = 0; i < arp->config->euclideanLengths; i++)
    {
        uint8_t x = i % 8;
        uint8_t y = i / 8;

        if(arp->Active() && arp->euclideanIndex == i)
        {
            MatrixOS::LED::SetColor(origin + Point(x, y), Color::White);
        }
        else if(arp->config->euclideanOffset == i)
        {
            MatrixOS::LED::SetColor(origin + Point(x, y), rootColor);
        }
        else if((arp->euclideanMap >> i) & 1)
        {
            MatrixOS::LED::SetColor(origin + Point(x, y), color);
        }
        else
        {
            MatrixOS::LED::SetColor(origin + Point(x, y), color.Dim());
        }
    }
    return true;
  }

  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {
    // No key interaction for visualizer
    if(keyInfo->State() == RELEASED && keyInfo->hold == false)
    {
        uint8_t i = xy.x + xy.y * 8;
        if(i < arp->config->euclideanLengths)
        {
            arp->config->euclideanOffset = i;
            arp->UpdateConfig();
        }
    }
    return false;
  }
};