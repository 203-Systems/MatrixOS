#include "SequenceEvent.h"
#include "SequenceData.h"

void SequenceEvent::ExecuteEvent(SequenceData& sequenceData, uint8_t track)
{
    switch(eventType)
    {
        case SequenceEventType::NoteEvent:
        {
            SequenceEventNote noteEvent = std::get<SequenceEventNote>(data);
            uint8_t channel = sequenceData.trackList[track].channel;
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
            uint8_t channel = sequenceData.trackList[track].channel;
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
