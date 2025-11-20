#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"

class SequenceVisualizer : public UIComponent {
    Sequencer* sequencer;
    vector<uint8_t>* stepSelected;
    std::unordered_map<uint8_t, uint8_t>* noteSelected;
    std::unordered_multiset<uint8_t>* noteActive;
    uint8_t width = 8;
    std::function<void(uint8_t)> selectCallback;

    public:
    SequenceVisualizer(Sequencer* sequencer, vector<uint8_t>* stepSelected, std::unordered_map<uint8_t, uint8_t>* noteSelected, std::unordered_multiset<uint8_t>* noteActive)
    {
        this->sequencer = sequencer;
        this->stepSelected = stepSelected;
        this->noteSelected = noteSelected;
        this->noteActive = noteActive;
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
        uint8_t track = sequencer->track;
        uint8_t patternIdx = sequencer->trackPatternIdx[track];
        uint8_t channel = sequencer->sequence.GetChannel(track);

        if(keyInfo->state == PRESSED)
        {
            // Check if step already exists in selection
            if(std::find(stepSelected->begin(), stepSelected->end(), step) == stepSelected->end())
            {
                stepSelected->push_back(step);
            }

            // Populate noteActive with notes from this step and send MIDI NoteOn
            uint8_t clip = sequencer->trackClipIdx[track];
            if(patternIdx < sequencer->sequence.GetPatternCount(track, clip))
            {
                SequencePattern& pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);
                uint16_t startTime = step * Sequence::PPQN;
                uint16_t endTime = startTime + Sequence::PPQN - 1;

                auto it = pattern.events.lower_bound(startTime);
                while (it != pattern.events.end() && it->first <= endTime)
                {
                    if (it->second.eventType == SequenceEventType::NoteEvent)
                    {
                        const SequenceEventNote& noteData = std::get<SequenceEventNote>(it->second.data);
                        noteActive->insert(noteData.note);
                        MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, noteData.note, noteData.velocity));
                    }
                    ++it;
                }
            }

            if(selectCallback != nullptr)
            {
                selectCallback(step);
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

            // Clear noteActive from notes in this step and send MIDI NoteOff
            uint8_t clip = sequencer->trackClipIdx[track];
            if(patternIdx < sequencer->sequence.GetPatternCount(track, clip))
            {
                SequencePattern& pattern = sequencer->sequence.GetPattern(track, clip, patternIdx);
                uint16_t startTime = step * Sequence::PPQN;
                uint16_t endTime = startTime + Sequence::PPQN - 1;

                auto it = pattern.events.lower_bound(startTime);
                while (it != pattern.events.end() && it->first <= endTime)
                {
                    if (it->second.eventType == SequenceEventType::NoteEvent)
                    {
                        const SequenceEventNote& noteData = std::get<SequenceEventNote>(it->second.data);
                        noteActive->erase(noteActive->find(noteData.note));

                        // Only send NoteOff if note is not currently selected on NotePad
                        if(noteSelected->find(noteData.note) == noteSelected->end())
                        {
                            MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, noteData.note, 0));
                        }
                    }
                    ++it;
                }
            }
        }

        return true;
    }

    virtual bool Render(Point origin)
    {
        uint8_t track = sequencer->track;
        uint8_t clip = sequencer->trackClipIdx[track];
        uint8_t patternIdx = sequencer->trackPatternIdx[track];

        // Check if pattern is selected
        if(patternIdx >= sequencer->sequence.GetPatternCount(track, clip))
        {
            sequencer->trackPatternIdx[track] = 0;
        }

        SequencePattern& pattern = sequencer->sequence.GetPattern(track, clip, (uint8_t)patternIdx);


        // Render base
        for(uint8_t step = 0; step < pattern.quarterNotes; step++)
        {
            Point point = Point(step % width, step / width);
            MatrixOS::LED::SetColor(origin + point, sequencer->meta.tracks[track].color.Dim());
        }

        // Render Note
        for(uint8_t slot = 0; slot < pattern.quarterNotes; slot++)
        {
            uint16_t startTime = slot * Sequence::PPQN;
            uint16_t endTime = startTime + Sequence::PPQN - 1;

            if(pattern.HasEventInRange(startTime, endTime, SequenceEventType::NoteEvent))
            {
                Point point = Point(slot % width, slot / width);
                MatrixOS::LED::SetColor(origin + point, sequencer->meta.tracks[track].color);
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
                MatrixOS::LED::SetColor(origin + point, sequencer->sequence.RecordEnabled() ? Color::Red : Color::Green);
            }
        }

        return true;
    }
};
