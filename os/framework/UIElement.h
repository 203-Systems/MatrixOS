class UIElement
{
    public:
    virtual string GetName(){return "Unnamed UI Element";}
    virtual Color GetColor(){return 0xFFFFFF;}
    virtual Dimension GetSize(){return Dimension(0,0);}
    virtual bool KeyEvent(Point xy, KeyInfo keyInfo) {return false;} //
    // virtual bool Callback(Point xy){return false;}
    // virtual bool HoldCallback(Point xy){return false;}

    virtual bool Render(Point origin){return false;}

    virtual ~UIElement() {};

    operator UIElement*() {return this;}
};

class UIButton : public UIElement
{
    public:
    string name;
    Color color;
    std::function<void()> callback;
    std::function<void()> hold_callback;

    UIButton(string name, Color color, std::function<void()> callback = nullptr, std::function<void()> hold_callback = nullptr)
    {
        this->name = name;
        this->color = color;
        this->callback = callback;
        this->hold_callback = hold_callback;
    }

    virtual string GetName() {return name;}
    virtual Color GetColor() {return color;}
    virtual Dimension GetSize() {return Dimension(1,1);}

    virtual bool Callback() {if(callback != nullptr){callback(); return true;} return false;}
    virtual bool HoldCallback() {if(hold_callback){hold_callback(); return true;} return false;}
    virtual bool Render(Point origin) {MatrixOS::LED::SetColor(origin, GetColor()); return true;}

    virtual bool KeyEvent(Point xy, KeyInfo keyInfo)
    {
        if (keyInfo.state == RELEASED && keyInfo.hold == false)
        {
            if (Callback())
            {
                MatrixOS::Logging::LogDebug("UI Button", "Key Event Callback");
                MatrixOS::KEYPAD::Clear();
                return true;
            }
        }
        else if (keyInfo.state == HOLD)
        {
            if (HoldCallback())
            {
                MatrixOS::KEYPAD::Clear();
                return true;
            }
            else
            {
                MatrixOS::UIComponent::TextScroll(GetName(), GetColor());
                return true;
            }
        }
        return false;
    }
};

class UIButtonLarge : public UIButton
{
    public:
    std::function<Color()> color_func;
    Dimension dimension;

    UIButtonLarge(string name, Color color, Dimension dimension, std::function<void()> callback = nullptr, std::function<void()> hold_callback = nullptr) 
    : UIButton(name, color, callback, hold_callback)
    {
        this->dimension = dimension;
    }

    virtual Dimension GetSize(){return dimension;}

    virtual bool Render(Point origin) {
        for(uint16_t x = 0; x < dimension.x; x++)
        {
            for(uint16_t y = 0; y < dimension.y; y++)
            {
                MatrixOS::LED::SetColor(origin + Point(x, y), color);
            }
        }
        return true;
    }
};


class UIButtonWithColorFunc : public UIButton
{
    public:
    std::function<Color()> color_func;

    UIButtonWithColorFunc(string name, std::function<Color()> color_func, std::function<void()> callback = nullptr, std::function<void()> hold_callback = nullptr) 
    : UIButton(name, Color(0xFFFFFF), callback, hold_callback)
    {
        this->color_func = color_func;
    }

    virtual Color GetColor() {return color_func();}
};

class UIButtonDimmable : public UIButton
{
    public:
    std::function<bool()> dim_func; //If this returns false, then dim button

    UIButtonDimmable(string name, Color color, std::function<bool()> dim_func, std::function<void()> callback = nullptr, std::function<void()> hold_callback = nullptr) 
    : UIButton(name, color, callback, hold_callback)
    {
        this->dim_func = dim_func;
    }

    virtual Color GetColor() {return color.ToLowBrightness(dim_func());}
};
