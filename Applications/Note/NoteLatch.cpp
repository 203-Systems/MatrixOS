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
        toggleMode = false;

        // Passthrough everything in input to output
        for (const MidiPacket& packet : input) {
            output.push_back(packet);
        }
        
        return;
    }

    for (const MidiPacket& packet : input) {
        if (packet.status == NoteOn || packet.status == NoteOff) {
            toggleMode ? ProcessNoteMessageToggleMode(packet, output) : ProcessNoteMessage(packet, output);
        } else if (packet.status == AfterTouch) {
            // Drop aftertouch packet
        }
        else
        {
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

void NoteLatch::SetToggleMode(bool enable) {
    if (enable) {
        enabled = true;
        toggleMode = true;
    } else if (enabled) {
        toggleMode = false;
    }
}

void NoteLatch::ProcessNoteMessage(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t note = packet.Note();
    lastChannel = packet.Channel();

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
        auto latchedIt = std::find(latchedNotes.begin(), latchedNotes.end(), note);
        auto holdingIt = std::find(stillHoldingNotes.begin(), stillHoldingNotes.end(), note);

        if (holdingIt != stillHoldingNotes.end()) { // If it was holding then we remove it from active keys
            stillHoldingNotes.erase(holdingIt);
        }
        else if(latchedIt == latchedNotes.end()) // If it was, then note latch is enabled after the key is held. Add this entry
        {
            latchedNotes.push_back(note);
        }
    }
}

void NoteLatch::ProcessNoteMessageToggleMode(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t note = packet.Note();

    if (packet.status == NoteOn && packet.Velocity() > 0) {
        // Note On - check if note is already latched
        auto latchedIt = std::find(latchedNotes.begin(), latchedNotes.end(), note);

        if (latchedIt != latchedNotes.end()) {
            // Note is already latched - send note off and remove from latched list
            MidiPacket noteOff = MidiPacket::NoteOff(packet.Channel(), note, 0);
            output.push_back(noteOff);
            latchedNotes.erase(latchedIt);
        } else {
            // Note is not latched - add to latched list and send note on
            latchedNotes.push_back(note);
            output.push_back(packet);
        }
    }
    else if (packet.status == NoteOff || (packet.status == NoteOn && packet.Velocity() == 0)) {
        // Note Off - ignore for toggle mode (notes are toggled by note on only)
    }
}

void NoteLatch::ReleaseAllLatchedNotes(deque<MidiPacket>& output) {
    // Send note off for all latched notes
    for (uint8_t note : latchedNotes) {
        MidiPacket noteOff = MidiPacket::NoteOff(lastChannel, note, 0); // Channel 0 for simplicity
        output.push_back(noteOff);
    }

    latchedNotes.clear();
    stillHoldingNotes.clear();
}