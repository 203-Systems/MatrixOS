#include "EventDetailView.h"

EventDetailView::EventDetailView(Sequencer* sequencer)
{
    this->sequencer = sequencer;
}

Dimension EventDetailView::GetSize()
{
    return Dimension(8, 7);
}

bool EventDetailView::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    return true;
}

bool EventDetailView::Render(Point origin)
{
    return true;
}

void EventDetailView::RenderNoteEvent(Point origin)
{
}

bool EventDetailView::NoteEventKeyHandler(Point xy, KeyInfo* keyInfo)
{
    return false;
}

void EventDetailView::RenderCCEvent(Point origin)
{
}

void EventDetailView::CCEventHandler(Point xy, KeyInfo* keyInfo)
{
}
