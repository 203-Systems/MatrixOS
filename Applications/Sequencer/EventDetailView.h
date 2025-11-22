#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"
#include "SequenceEvent.h"

class EventDetailView : public UIComponent {
    Sequencer* sequencer;
    SequenceEvent* event = nullptr;
    bool hasEvent = false;

    // UI parameters for displaying/editing event properties
    uint8_t selectedField = 0; // Which field is being edited
    uint8_t numFields = 0;     // Total editable fields (depends on event type)

public:
    EventDetailView(Sequencer* sequencer)
    {
        this->sequencer = sequencer;
    }

    void SetEvent(SequenceEvent* evt)
    {
        event = evt;
        hasEvent = (evt != nullptr);
        selectedField = 0;

        if (hasEvent)
        {
            // Calculate number of editable fields based on event type
            switch (event->GetType())
            {
                case SequenceEventType::NoteEvent:
                    numFields = 4; // note, velocity, length, aftertouch
                    break;
                case SequenceEventType::ControlChangeEvent:
                    numFields = 2; // param, value
                    break;
                default:
                    numFields = 0;
                    break;
            }
        }
    }

    void ClearEvent()
    {
        event = nullptr;
        hasEvent = false;
        selectedField = 0;
        numFields = 0;
    }

    Dimension GetSize() { return Dimension(8, 8); }

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo)
    {
        if (!hasEvent)
            return true;

        // Allow selecting different fields (left column)
        if (keyInfo->State() == PRESSED && xy.x == 0 && xy.y < numFields)
        {
            selectedField = xy.y;
            return true;
        }

        // TODO: Add value editing controls for selected field

        return true;
    }

    virtual bool Render(Point origin)
    {
        if (!hasEvent || event == nullptr)
        {
            // Show "No Event Selected" message
            MatrixOS::UIUtility::DrawText(origin, "No Event", Color(0x404040));
            return true;
        }

        switch (event->GetType())
        {
            case SequenceEventType::NoteEvent:
                RenderNoteEvent(origin);
                break;
            case SequenceEventType::ControlChangeEvent:
                RenderCCEvent(origin);
                break;
            default:
                MatrixOS::UIUtility::DrawText(origin, "Invalid", Color(0xFF0000));
                break;
        }

        return true;
    }

private:
    void RenderNoteEvent(Point origin)
    {
        SequenceEventNote& noteData = event->data.note;
        Color trackColor = sequencer->meta.tracks[sequencer->track].color;
        Color dimColor = trackColor.Dim();
        Color selectedColor = Color::White;

        // Field 0: Note number
        Color noteColor = (selectedField == 0) ? selectedColor : trackColor;
        MatrixOS::LED::SetColor(origin + Point(0, 0), noteColor);
        // TODO: Display note value (0-1) using remaining pixels

        // Field 1: Velocity
        Color velocityColor = (selectedField == 1) ? selectedColor : trackColor;
        MatrixOS::LED::SetColor(origin + Point(0, 1), velocityColor);
        // TODO: Display velocity bar graph

        // Field 2: Length (in pulses)
        Color lengthColor = (selectedField == 2) ? selectedColor : trackColor;
        MatrixOS::LED::SetColor(origin + Point(0, 2), lengthColor);
        // TODO: Display length value

        // Field 3: Aftertouch enabled
        Color aftertouchColor = (selectedField == 3) ? selectedColor : trackColor;
        if (noteData.aftertouch)
            MatrixOS::LED::SetColor(origin + Point(0, 3), aftertouchColor);
        else
            MatrixOS::LED::SetColor(origin + Point(0, 3), dimColor);
    }

    void RenderCCEvent(Point origin)
    {
        SequenceEventCC& ccData = event->data.cc;
        Color trackColor = sequencer->meta.tracks[sequencer->track].color;
        Color selectedColor = Color::White;

        // Field 0: CC Parameter number
        Color paramColor = (selectedField == 0) ? selectedColor : trackColor;
        MatrixOS::LED::SetColor(origin + Point(0, 0), paramColor);
        // TODO: Display parameter value

        // Field 1: CC Value
        Color valueColor = (selectedField == 1) ? selectedColor : trackColor;
        MatrixOS::LED::SetColor(origin + Point(0, 1), valueColor);
        // TODO: Display value bar graph
    }
};
