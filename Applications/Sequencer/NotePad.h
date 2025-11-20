#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"
#include "Scales.h"

class NotePad : public UIComponent {
    Sequencer* sequencer;
    vector<uint8_t>* noteSelected;
    uint8_t width = 8;
    std::function<void(uint8_t)> selectCallback;

    public:
    NotePad(Sequencer* sequencer, vector<uint8_t>* noteSelected);

    void OnSelect(std::function<void(uint8_t)> callback);

    Dimension GetSize();
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
