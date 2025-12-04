#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"

class SequencerNotePad; // Forward declaration

class SequencerControlBar : public UIComponent {
    Sequencer* sequencer;
    SequencerNotePad* notePad;

    const uint8_t OctaveGradient[8]  = {0, 16, 42, 68, 124, 182, 255};

    bool HandleShiftKey(uint8_t idx, bool right, KeyInfo* keyInfo);
    bool HandlePlayKey(KeyInfo* keyInfo);
    bool HandleResumeKey(KeyInfo* keyInfo);
    bool HandleRecordKey(KeyInfo* keyInfo);
    bool HandleSessionKey(KeyInfo* keyInfo);
    bool HandleMixerKey(KeyInfo* keyInfo);
    bool HandleClearKey(KeyInfo* keyInfo);
    bool HandleCopyKey(KeyInfo* keyInfo);
    bool HandleNudgeKey(bool positive, KeyInfo* keyInfo);
    bool HandleOctaveOffsetKey(bool positive, KeyInfo* keyInfo);
    bool HandleStepOctaveOffsetKey(bool positive, KeyInfo* keyInfo);
    bool HandleQuantizeKey(KeyInfo* keyInfo);
    bool HandleTwoPatternToggleKey(KeyInfo* keyInfo);

    public:
    SequencerControlBar(Sequencer* sequencer, SequencerNotePad* notePad);

    Dimension GetSize();
    Color GetOctavePlusColor();
    Color GetOctaveMinusColor();
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
