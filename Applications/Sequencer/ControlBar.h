#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"

class SequencerNotePad; // Forward declaration

class ControlBar : public UIComponent {
    Sequencer* sequencer;
    SequencerNotePad* notePad;
    std::function<void()> clearCallback;

    const uint8_t OctaveGradient[8]  = {0, 16, 42, 68, 124, 182, 255};

    public:
    ControlBar(Sequencer* sequencer, SequencerNotePad* notePad);

    void OnClear(std::function<void()> callback);

    Dimension GetSize();
    Color GetOctavePlusColor();
    Color GetOctaveMinusColor();
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
