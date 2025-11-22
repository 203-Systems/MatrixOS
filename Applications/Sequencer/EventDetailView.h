#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"
#include "SequenceEvent.h"
#include "SequenceData.h"

class EventDetailView : public UIComponent {
    Sequencer* sequencer;

    bool wasEnabled = false;

    uint8_t eventIndex;
    vector<std::multimap<uint16_t, SequenceEvent>::iterator> eventRefs;

    // UI parameters for displaying/editing event properties
    uint8_t selectedField = 0; // Which field is being edited
    uint8_t numFields = 0;     // Total editable fields (depends on event type)

public:
    EventDetailView(Sequencer* sequencer);

    virtual bool IsEnabled();
    Dimension GetSize();

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);

    virtual bool Render(Point origin);

private:
    void RebuildEventList();

    void RenderNoteEvent(Point origin);
    bool NoteEventKeyHandler(Point xy, KeyInfo* keyInfo);

    void RenderCCEvent(Point origin);
    void CCEventHandler(Point xy, KeyInfo* keyInfo);
};
