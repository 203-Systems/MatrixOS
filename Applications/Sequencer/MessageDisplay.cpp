#include "MessageDisplay.h"


SequencerMessageDisplay::SequencerMessageDisplay(Sequencer* sequencer)
{
    this->sequencer = sequencer;
}

bool SequencerMessageDisplay::IsEnabled()
{
    bool inSequencer = sequencer->currentView == Sequencer::ViewMode::Sequencer;

    if(inSequencer == false) {return false;}

    uint32_t currentTime = MatrixOS::SYS::Millis();

    bool hasMessage = sequencer->lastMessage != SequencerMessage::NONE && sequencer->lastMessageTime != 0;
    bool messagePersistent = sequencer->lastMessageTime == UINT32_MAX;
    bool messageTimedOut = (currentTime - sequencer->lastMessageTime) >= 1000;
    bool lastMessageRecent = hasMessage && (messagePersistent || !messageTimedOut);

    bool modifierActive = sequencer->ClearActive() || sequencer->CopyActive();
    bool shiftEnabled = sequencer->ShiftActive() && ((currentTime - sequencer->shiftOnTime) > 150);
    
    return lastMessageRecent || modifierActive || shiftEnabled;
}

bool SequencerMessageDisplay::TwoRowMode()
{
    return sequencer->patternViewActive || sequencer->meta.tracks[sequencer->track].twoPatternMode;
}

Dimension SequencerMessageDisplay::GetSize()
{
    return Dimension(8, 4);
}

bool SequencerMessageDisplay::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    return false;
}

void SequencerMessageDisplay::ClearRows(Point origin, uint8_t row)
{
    for(uint8_t y = 0; y < row; y++)
    {
        for (uint8_t x = 0; x < 8; x++)
        {
            MatrixOS::LED::SetColor(origin + Point(x, y), Color::Black);
        }
    }
}

void SequencerMessageDisplay::RenderWave(Point origin, uint8_t row, Color color)
{
    ClearRows(origin, row);
    for(uint8_t x = 0; x < 8; x++)
    {
        uint8_t scale = ColorEffects::Breath(600, 75 * x);
        Color colColor = Color::Crossfade(color, Color::White, Fract16(scale, 8));
        for(uint8_t y = 0; y < row; y++)
        {
            MatrixOS::LED::SetColor(origin + Point(x, y), colColor);
        }
    }
}

void SequencerMessageDisplay::RenderClear(Point origin, Color color)
{
    if(TwoRowMode())
    {
        RenderWave(origin + Point(0, 2), 2, color);
        return;
    }

    ClearRows(origin, 4);

    // C
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // L
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);                

    // R
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
}

void SequencerMessageDisplay::RenderCopy(Point origin, Color color)
{
    if(TwoRowMode())
    {
        RenderWave(origin + Point(0, 2), 2, color);
        return;
    }

    ClearRows(origin, 4);

    // C
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // P
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);                

    // Y
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
}

void SequencerMessageDisplay::RenderShift(Point origin, Color color)
{
    RenderWave(origin + Point(0, 2), 2, color);
}

void SequencerMessageDisplay::RenderNudge(Point origin, Color color)
{
    ClearRows(origin, 4);

    // n
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // u
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 3), Color::White);

    // d
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
}

void SequencerMessageDisplay::RenderQuantize(Point origin, Color color)
{
    ClearRows(origin, 4);

    // q
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);

    // u
    MatrixOS::LED::SetColor(origin + Point(2, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // n
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
}

void SequencerMessageDisplay::Render2PatternView(Point origin, Color color)
{
    ClearRows(origin, 4);

    // 2
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);

    // P
    MatrixOS::LED::SetColor(origin + Point(2, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);

    // V
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
}

void SequencerMessageDisplay::RenderClip(Point origin, Color color)
{
    ClearRows(origin, 4);

    // C
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // L
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // P
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
}

void SequencerMessageDisplay::RenderMix(Point origin, Color color)
{
    ClearRows(origin, 4);

    // M
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // I
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // X
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
}

void SequencerMessageDisplay::RenderPlay(Point origin, Color color)
{
    ClearRows(origin, 4);

    // P
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);

    // L
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // Y
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
}

void SequencerMessageDisplay::RenderRecord(Point origin, Color color)
{
    ClearRows(origin, 4);

    // R
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);
    
    // E
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 3), Color::White);

    // C
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);

}

void SequencerMessageDisplay::RenderUndo(Point origin, Color color)
{
    ClearRows(origin, 4);

    // U
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);
    
    // d
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // o
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
}

bool SequencerMessageDisplay::Render(Point origin)
{
    const Color successColor = Color(0x80FF00);
    bool hasMessage = sequencer->lastMessage != SequencerMessage::NONE && sequencer->lastMessageTime != 0;
    bool messagePersistent = sequencer->lastMessageTime == UINT32_MAX;
    bool messageTimedOut = (MatrixOS::SYS::Millis() - sequencer->lastMessageTime) >= 1000;
    bool lastMessageRecent = hasMessage && (messagePersistent || !messageTimedOut);
    if(lastMessageRecent && sequencer->lastMessage != SequencerMessage::NONE)
    {
        switch(sequencer->lastMessage)
        {
            case SequencerMessage::CLEAR:
            case SequencerMessage::CLEARED:
            {
                Color copyColor = sequencer->lastMessage == SequencerMessage::CLEAR ? Color(0xFF0080) : successColor;
                RenderClear(origin, copyColor);
                break;
            }
            case SequencerMessage::COPY:
            case SequencerMessage::COPIED:
            {
                Color copyColor = sequencer->lastMessage == SequencerMessage::COPY ? Color(0x0080FF) : successColor;
                RenderCopy(origin, copyColor);
                break;
            }
            case SequencerMessage::NUDGE:
            {
                Color nudgeColor = Color(0xFF0040);
                RenderNudge(origin, nudgeColor);
                break;
            }
            case SequencerMessage::QUANTIZE:
            case SequencerMessage::QUANTIZED:
            {
                Color quantizedColor = sequencer->lastMessage == SequencerMessage::QUANTIZE ? Color(0xA000FF) : successColor;
                RenderQuantize(origin, quantizedColor);
                break;
            }
            case SequencerMessage::TWO_PATTERN_VIEW:
            {
                Color twoPatternColor = Color(0xFFFF00);
                Render2PatternView(origin, twoPatternColor);
                break;
            }
            case SequencerMessage::CLIP:
            {
                Color clipColor = Color(0xFFFF00);
                RenderClip(origin, clipColor);
                break;
            }
            case SequencerMessage::MIX:
            {
                Color mixColor = Color(0x40FF00);
                RenderMix(origin, mixColor);
                break;
            }
            case SequencerMessage::PLAY:
            {
                Color playColor = Color(0x00FF00);
                RenderPlay(origin, playColor);
                break;
            }
            case SequencerMessage::RECORD:
            {
                Color recordColor = Color(0xFF0000);
                RenderRecord(origin, recordColor);
                break;
            }
            case SequencerMessage::UNDO:
            {
                Color undoColor = Color(0xFF0040);
                RenderUndo(origin, undoColor);
                break;
            }
        }
    }
    else if(sequencer->ClearActive())
    {
        RenderClear(origin, Color(0xFF0080));
    }
    else if(sequencer->CopyActive())
    {
        RenderCopy(origin, Color(0x0080FF));
    }
    else if(sequencer->ShiftActive() && ((MatrixOS::SYS::Millis() - sequencer->shiftOnTime) > 150))
    {
        RenderShift(origin, sequencer->meta.tracks[sequencer->track].color);
    }
    return true;
}
