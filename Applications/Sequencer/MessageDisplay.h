#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"


class SequencerMessageDisplay : public UIComponent {
    Sequencer* sequencer;

    bool IsEnabled();
    bool TwoRowMode();
    void ClearRows(Point origin, uint8_t row);
    void RenderWave(Point origin, uint8_t row, Color color, uint16_t speed = 600, uint16_t xDelay = 80);
    void RenderClear(Point origin, Color color);
    void RenderCopy(Point origin, Color color, bool fast);
    void RenderShift(Point origin, Color color);
    void RenderNudge(Point origin, Color color);
    void RenderQuantize(Point origin, Color color);
    void Render2PatternView(Point origin, Color color);
    void RenderClip(Point origin, Color color);
    void RenderMix(Point origin, Color color);
    void RenderPlay(Point origin, Color color);
    void RenderRecord(Point origin, Color color);
    void RenderUndo(Point origin, Color color);

    public:
    SequencerMessageDisplay(Sequencer* sequencer);

    Dimension GetSize();
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
