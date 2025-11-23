#include "SequenceEvent.h"
#include "SequenceData.h"
#include "Sequence.h"

SequenceEvent SequenceEvent::Note(const uint8_t note, const uint8_t velocity, const bool aftertouch, const uint16_t length)
{
    // Auto-set length to default step (PPQN/4) if UINT16_MAX is passed
    uint16_t actualLength = length;
    if (length == UINT16_MAX) { actualLength = Sequence::PPQN / 4;}
    return {SequenceEventType::NoteEvent, SequenceEventNote{aftertouch, note, velocity, actualLength}};
}

SequenceEvent SequenceEvent::ControlChange(const uint8_t param, const uint8_t value)
{
    return {SequenceEventType::ControlChangeEvent, SequenceEventCC{param, value}};
}

// SequenceEvent SequenceEvent::ProgramChange(const uint8_t program)
// {
//     return {SequenceEventType::ProgramControl, SequenceEventPC{program}};
// }

// SequenceEvent SequenceEvent::BPMChange(const uint16_t bpm)
// {
//     return {SequenceEventType::BPMChange, SequenceEventBPMChange{bpm}};
// }

// SequenceEvent SequenceEvent::SwingChange(const uint8_t swing)
// {
//     return {SequenceEventType::SwingChange, SequenceEventSwingChange{swing}};
// }

void SequenceEvent::ExecuteEvent(SequenceData& sequenceData, uint8_t track)
{
    switch(eventType)
    {
        case SequenceEventType::NoteEvent:
        {
            SequenceEventNote noteEvent = std::get<SequenceEventNote>(data);
            uint8_t channel = sequenceData.tracks[track].channel;
            if(noteEvent.aftertouch)
            {
                MatrixOS::MIDI::Send(MidiPacket::AfterTouch(channel, noteEvent.note, noteEvent.velocity), MIDI_PORT_ALL);
            }
            else
            {
                MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, noteEvent.note, noteEvent.velocity), MIDI_PORT_ALL);
            }
            break;
        }
        case SequenceEventType::ControlChangeEvent:
        {
            SequenceEventCC ccEvent = std::get<SequenceEventCC>(data);
            uint8_t channel = sequenceData.tracks[track].channel;
            MatrixOS::MIDI::Send(MidiPacket::ControlChange(channel, ccEvent.param, ccEvent.value), MIDI_PORT_ALL);
            break;
        }
        // case SequenceEventType::ProgramControlEvent:
        // {
        //     SequenceEventPC pcEvent = std::get<SequenceEventPC>(data);
        //     MatrixOS::MIDI::Send(MidiPacket::ProgramChange(channel, pcEvent.program), MIDI_PORT_ALL);
        //     break;
        // }
        // case SequenceEventType::BPMChangeEvent:
        // {
        //     SequenceEventBPMChange bpmEvent = std::get<SequenceEventBPMChange>(data);
        //     sequenceData.bpm = bpmEvent.bpm;
        //     break;
        // }
        // case SequenceEventType::SwingChangeEvent:
        // {
        //     SequenceEventSwingChange swingEvent = std::get<SequenceEventSwingChange>(data);
        //     sequenceData.swing = swingEvent.swing;
        //     break;
        // }
        default:
            break;
    }
}
