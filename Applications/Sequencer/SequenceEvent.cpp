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
