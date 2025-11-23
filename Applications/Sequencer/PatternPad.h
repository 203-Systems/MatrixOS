#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>
#include <set>

#include "Sequencer.h"

class PatternPad : public UIComponent {
    Sequencer* sequencer;
    std::set<uint8_t>* stepSelected;
    std::unordered_map<uint8_t, uint8_t>* noteSelected;
    std::unordered_multiset<uint8_t>* noteActive;
    uint8_t width = 8;

    public:
    PatternPad(Sequencer* sequencer, std::set<uint8_t>* stepSelected, std::unordered_map<uint8_t, uint8_t>* noteSelected, std::unordered_multiset<uint8_t>* noteActive);

    Dimension GetSize();
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
