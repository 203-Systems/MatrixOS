#pragma once
#include "MatrixOS.h"

class UIComponent
{
    public:
    virtual Dimension GetSize(){return Dimension(0,0);}
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) {return false;} //
    // virtual bool Callback(Point xy){return false;}
    // virtual bool HoldCallback(Point xy){return false;}

    virtual bool Render(Point origin){return false;}

    virtual ~UIComponent() {};

    operator UIComponent*() {return this;}
};

