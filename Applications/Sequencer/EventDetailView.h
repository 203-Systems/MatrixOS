#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"
#include "SequenceEvent.h"
#include "SequenceData.h"

class EventDetailView : public UIComponent {
    Sequencer* sequencer;

    bool wasEnabled = false;

    vector<std::multimap<uint16_t, SequenceEvent>::iterator> eventRefs;
    std::multimap<uint16_t, SequenceEvent>::iterator selectedEventIter;

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

    // Event selector (Y=0 row)
    void RenderEventSelector(Point origin);
    bool EventSelectorKeyHandler(Point xy, KeyInfo* keyInfo);

    // Micro step selector (Y=1 row)
    void RenderMicroStepSelector(Point origin);
    bool MicroStepSelectorKeyHandler(Point xy, KeyInfo* keyInfo);
    bool DeleteEventKeyHandler(Point xy, KeyInfo* keyInfo);

    // Note event configuration (Y >= 2)
    void RenderNoteConfig(Point origin);
    bool NoteConfigKeyHandler(Point xy, KeyInfo* keyInfo);

    // CC event configuration (Y >= 2)
    void RenderCCConfig(Point origin);
    bool CCConfigKeyHandler(Point xy, KeyInfo* keyInfo);
};
