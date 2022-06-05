class UIElement
{
    public:
    virtual string GetName(){return NULL;}
    virtual Color GetColor(){return 0xFFFFFF;}
    virtual Dimension GetSize(){return Dimension(0,0);}
    virtual bool Callback(){return false;}
    virtual bool HoldCallback(){return false;}

    virtual bool Render(Point origin){return false;}

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
    virtual bool Render(Point origin) {MatrixOS::LED::SetColor(origin, color); return true;}


};

class UIButtonWithColorFunc : public UIElement
{
    public:
    string name;
    std::function<Color()> color_func;
    std::function<void()> callback;
    std::function<void()> hold_callback;

    UIButtonWithColorFunc(string name, std::function<Color()> color_func, std::function<void()> callback = nullptr, std::function<void()> hold_callback = nullptr)
    {
        this->name = name;
        this->color_func = color_func;
        this->callback = callback;
        this->hold_callback = hold_callback;
    }

    string GetName() override {return name;}
    Color GetColor() override {return color_func();}
    Dimension GetSize() override {return Dimension(1,1);}
    virtual bool Callback() override {if(callback){callback(); return true;} return false;}
    virtual bool HoldCallback() override {if(hold_callback){hold_callback(); return true;} return false;}
    bool Render(Point origin){MatrixOS::LED::SetColor(origin, color_func()); return true;}
};
