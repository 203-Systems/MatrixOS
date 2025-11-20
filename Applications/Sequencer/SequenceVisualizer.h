#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class SequenceVisualizer : public UIComponent {
    Sequencer* sequencer;
    vector<uint8_t>* stepSelected;
    uint8_t width = 8;
    std::function<void(uint8_t)> selectCallback;

    public:
    SequenceVisualizer(Sequencer* sequencer, vector<uint8_t>* stepSelected)
    {
        this->sequencer = sequencer;
        this->stepSelected = stepSelected;
        width = sequencer->sequence.GetTrackCount();
    }

    void OnSelect(std::function<void(uint8_t)> callback)
    {
        selectCallback = callback;
    }

    Dimension GetSize() { return Dimension(8, 2); }
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo)
    {
        uint8_t step = xy.x + xy.y * width;

        if(keyInfo->state == PRESSED)
        {
            // Check if step already exists in selection
            if(std::find(stepSelected->begin(), stepSelected->end(), step) == stepSelected->end())
            {
                stepSelected->push_back(step);
                if(selectCallback != nullptr)
                {
                    selectCallback(step);
                }
            }
        }
        else if(keyInfo->state == RELEASED)
        {
            // Remove step from selection
            auto it = std::find(stepSelected->begin(), stepSelected->end(), step);
            if(it != stepSelected->end())
            {
                stepSelected->erase(it);
            }
        }

        return true;
    }

    virtual bool Render(Point origin)
    {
        uint8_t track = sequencer->track;

        int8_t patternIdx = (uint8_t)sequencer->trackPatternIdx[track];

        // Check if pattern is selected
        if(patternIdx < 0 && patternIdx >= (int8_t)sequencer->sequence.GetPatternCount(track))
        {
            return true;
        }

        SequencePattern& pattern = sequencer->sequence.GetPattern(track, (uint8_t)patternIdx);


        // Render base
        for(uint8_t step = 0; step < pattern.quarterNotes; step++)
        {
            Point point = Point(step % width, step / width);
            MatrixOS::LED::SetColor(origin + point, sequencer->meta.tracks[track].color.Dim());
        }

        // Render Note
        int8_t lastRenderedSlot = -1;
        for(const auto& [time, event] : pattern.events)
        {
            // Check if this is a note event
            if(event.eventType == SequenceEventType::NoteEvent)
            {
                // Get the note data
                const SequenceEventNote& noteData = std::get<SequenceEventNote>(event.data);
                uint8_t note = noteData.note;

                uint8_t slot = time / Sequence::PPQN;
                if(slot != lastRenderedSlot)
                {
                    lastRenderedSlot = slot;
                    Point point = Point(slot % width, slot / width);
                    MatrixOS::LED::SetColor(origin + point, sequencer->meta.tracks[track].color);
                }
            }
        }

        // Render Selected
        for(uint8_t selectedStep : *stepSelected)
        {
            Point point = Point(selectedStep % width, selectedStep / width);
            MatrixOS::LED::SetColor(origin + point, Color::White);
        }

        // Render Cursor
        if(sequencer->sequence.Playing())
        {
            SequencePosition& position = sequencer->sequence.GetPosition(track);
            if(patternIdx == position.pattern)
            {
                uint8_t slot = position.quarterNote;
                Point point = Point(slot % width, slot / width);
                MatrixOS::LED::SetColor(origin + point, sequencer->sequence.RecordEnabled() ? Color::Red : Color::White);
            }
        }

        return true;
    }
};
