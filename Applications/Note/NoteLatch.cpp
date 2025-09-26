#include "NoteLatch.h"
#include <algorithm>

void NoteLatch::Tick(deque<MidiPacket>& input, deque<MidiPacket>& output) {
    // Check if we need to disable and release latched notes
    if (disableOnNextTick) {
        if (!latchedNotes.empty()) {
            ReleaseAllLatchedNotes(output);
        }
        
        enabled = false;
        disableOnNextTick = false;

        // Passthrough everything in input to output
        for (const MidiPacket& packet : input) {
            output.push_back(packet);
        }
        
        return;
    }

    for (const MidiPacket& packet : input) {
        if (packet.status == NoteOn || packet.status == NoteOff) {
            ProcessNoteMessage(packet, output);
        } else if (packet.status == AfterTouch) {
            ProcessAfterTouch(packet, output);
        } else {
            // Pass through other messages unchanged
            output.push_back(packet);
        }
    }
}

void NoteLatch::Reset() {
    // Clean up by clearing state
    latchedNotes.clear();
    stillHoldingNotes.clear();
}

void NoteLatch::SetEnabled(bool state) {
    if (state) {
        enabled = true;
    } else if (enabled) {
        disableOnNextTick = true;
    }
}

void NoteLatch::ProcessNoteMessage(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t note = packet.Note();

    if (packet.status == NoteOn && packet.Velocity() > 0) {
        // Note On
        if (stillHoldingNotes.empty() && !latchedNotes.empty()) {
            // There are active latched notes - release them all
            ReleaseAllLatchedNotes(output);
        }

        // No active latched notes - latch this note
        latchedNotes.push_back(note);
        stillHoldingNotes.push_back(note);

        // Send the note on
        output.push_back(packet);
    }
    else if (packet.status == NoteOff || (packet.status == NoteOn && packet.Velocity() == 0)) {
        // Note Off
        auto holdingIt = std::find(stillHoldingNotes.begin(), stillHoldingNotes.end(), note);

        if (holdingIt != stillHoldingNotes.end()) {
            stillHoldingNotes.erase(holdingIt);
        }
    }
}

void NoteLatch::ProcessAfterTouch(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t note = packet.Note();

    // Check if this aftertouch is from the first still holding note
    if (!stillHoldingNotes.empty() && stillHoldingNotes[0] == note) {
        // Mirror this aftertouch to all latched notes
        for (uint8_t latchedNote : latchedNotes) {
            MidiPacket mirroredAfterTouch = MidiPacket::AfterTouch(
                packet.Channel(),  // Use same channel as input
                latchedNote,
                packet.data[2] // pressure value
            );
            output.push_back(mirroredAfterTouch);
        }
    }
}

void NoteLatch::ReleaseAllLatchedNotes(deque<MidiPacket>& output) {
    // Send note off for all latched notes
    for (uint8_t note : latchedNotes) {
        MidiPacket noteOff = MidiPacket::NoteOff(0, note, 0); // Channel 0 for simplicity
        output.push_back(noteOff);
    }

    latchedNotes.clear();
    stillHoldingNotes.clear();
}