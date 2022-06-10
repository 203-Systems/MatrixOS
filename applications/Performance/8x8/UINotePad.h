#include "UI.h"

template<uint8_t tx, uint8_t ty>
class UINotePad : public UIElement
{
    public:
    string name;
    Color color;
    uint8_t channel;
    uint8_t (*map)[tx][ty];

    UINotePad(string name, Color color, uint8_t channel, uint8 map[tx][ty])
    {
        this->name = name;
        this->color = color;
        this->channel = channel;
        this->map = map;
    }



    virtual string GetName() {return name;}
    virtual Color GetColor() {return color;}
    virtual Dimension GetSize() {return Dimension(1,1);}
    virtual bool Callback() {if(return true;}
    virtual bool HoldCallback() {if return true;}
    virtual bool Render(Point origin) 
    {
        for(uint8_t x = 0; x < tx; x++)
        {
            for(uint8_t y = 0; y < ty; y++)
            {
                Point target_coord = origin + Point(x, y);
                Color target_color = MatrixOS::KEYPAD::GetKey(target_coord) ? Color(0xFFFFFF) : color;
                MatrixOS::LED::SetColor(target_coord,  target_color);
            }
        }    
        return true;
    }
};