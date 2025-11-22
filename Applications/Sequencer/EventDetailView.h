#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"
#include "SequenceEvent.h"

class EventDetailView : public UIComponent {
    Sequencer* sequencer;
    uint8_t eventIndex;

    // UI parameters for displaying/editing event properties
    uint8_t selectedField = 0; // Which field is being edited
    uint8_t numFields = 0;     // Total editable fields (depends on event type)

public:
    EventDetailView(Sequencer* sequencer);

    Dimension GetSize();

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);

    virtual bool Render(Point origin);

private:
    void RenderNoteEvent(Point origin);
    bool NoteEventKeyHandler(Point xy, KeyInfo* keyInfo);

    void RenderCCEvent(Point origin);
    void CCEventHandler(Point xy, KeyInfo* keyInfo);
};
