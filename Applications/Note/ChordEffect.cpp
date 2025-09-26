#include "ChordEffect.h"
#include <algorithm>

void ChordEffect::Tick(deque<MidiPacket>& input, deque<MidiPacket>& output) {
    if (!enabled) {
        output = std::move(input);
        return;
    }

    if (disableOnNextTick) {
        disableOnNextTick = false;
        enabled = false;

        // Release all chords
        ReleaseAllChords(output);

        // Pass through remaining input
        for (const MidiPacket& packet : input) {
            output.push_back(packet);
        }
        input.clear();
        return;
    }

    // If chord combo changed, update all active chords
    if (ChordComboChanged) {
        UpdateChords(output);
        ChordComboChanged = false;
    }

    // Process each packet
    for (const MidiPacket& packet : input) {
        if (packet.status == NoteOn || packet.status == NoteOff) {
            if (packet.status == NoteOn && packet.Velocity() > 0) {
                ProcessNoteOn(packet, output);
            } else {
                ProcessNoteOff(packet, output);
            }
        } else if (packet.status == AfterTouch) {
            ProcessAfterTouch(packet, output);
        } else {
            output.push_back(packet);
        }
    }
    input.clear();
}

void ChordEffect::ProcessNoteOn(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t root = packet.Note();
    uint8_t velocity = packet.Velocity();
    uint8_t channel = packet.Channel();

    // Initialize NoteData for this root note
    NoteData& data = noteMap[root];
    data.velocity = velocity;
    data.chordNotes.clear();

    // Add to note order if not already present
    auto it = std::find(noteOrder.begin(), noteOrder.end(), root);
    if (it == noteOrder.end()) {
        noteOrder.push_back(root);
    }

    // Output root note
    output.push_back(packet);

    // Calculate and generate chord notes based on chord bitmap
    for (uint8_t interval = 1; interval < 16; interval++) {
        if (chord & (1 << interval)) {
            uint8_t chordNote = root + interval;
            if (chordNote < 128) {
                // Check if another root note owns this chord note
                if (noteOwner.find(chordNote) != noteOwner.end()) {
                    uint8_t previousOwner = noteOwner[chordNote];
                    // Remove from previous owner's vector
                    auto& prevData = noteMap[previousOwner];
                    prevData.chordNotes.erase(std::remove(prevData.chordNotes.begin(), prevData.chordNotes.end(), chordNote), prevData.chordNotes.end());
                }

                // Add to current root's chord notes
                data.chordNotes.push_back(chordNote);
                // Update reverse lookup
                noteOwner[chordNote] = root;

                // Send note on
                output.push_back(MidiPacket::NoteOn(channel, chordNote, velocity));
            }
        }
    }
}

void ChordEffect::ProcessNoteOff(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t root = packet.Note();
    uint8_t channel = packet.Channel();

    // Check if this root note exists in noteMap
    if (noteMap.find(root) == noteMap.end()) return;

    // Send note off for root
    output.push_back(packet);

    // Send note off for all chord notes in the noteMap under this root
    for (uint8_t chordNote : noteMap[root].chordNotes) {
        output.push_back(MidiPacket::NoteOff(channel, chordNote, 0));
        // Remove from reverse lookup
        noteOwner.erase(chordNote);
    }

    // Clear the root's entry from noteMap
    noteMap.erase(root);

    // Remove from note order
    noteOrder.erase(std::remove(noteOrder.begin(), noteOrder.end(), root), noteOrder.end());
}

void ChordEffect::ProcessAfterTouch(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t root = packet.Note();
    uint8_t velocity = packet.Velocity();
    uint8_t channel = packet.Channel();

    // Check if this root note exists in noteMap
    if (noteMap.find(root) == noteMap.end()) return;

    // Update velocity based on velocity (aftertouch modulates velocity)
    noteMap[root].velocity = velocity;

    // Pass through aftertouch for root note
    output.push_back(packet);

    // Send aftertouch for all chord notes in the noteMap under this root
    for (uint8_t chordNote : noteMap[root].chordNotes) {
        output.push_back(MidiPacket::AfterTouch(channel, chordNote, velocity));
    }
}

void ChordEffect::Reset() {
    noteMap.clear();
    noteOwner.clear();
    noteOrder.clear();
    chord = 0;
    ChordComboChanged = false;
    disableOnNextTick = false;
}

void ChordEffect::SetEnabled(bool state) {
    if (!state && enabled) {
        // Schedule graceful disable on next tick
        disableOnNextTick = true;
    } else {
        enabled = state;
        if (state) {
            disableOnNextTick = false;
        }
    }
}

void ChordEffect::SetChordCombo(ChordCombo combo) {
    chordCombo = combo;
    chord = CalculateChord(combo);
    ChordComboChanged = true;
}

uint16_t ChordEffect::CalculateChord(ChordCombo combo) {
    uint16_t result = 0; // Don't include root in bitmap since we handle it separately

    // Base triads
    if (combo.dim) {
        result |= 0b0000000001001000;  // Diminished: b3(3) b5(6)
    }

    if (combo.min) {
        result |= 0b0000000010001000;  // Minor: b3(3) 5(7)
    }

    if (combo.maj) {
        result |= 0b0000000010010000;  // Major: 3(4) 5(7)
    }

    if (combo.sus) {
        result |= 0b0000000010100000;  // Sus4: 4(5) 5(7)
    }

    // Extensions
    if (combo.ext6) {
        result |= 0b0000001000000000;  // 6th: 6(9)
    }
    if (combo.extMin7) {
        result |= 0b0000010000000000;  // Minor 7th: b7(10)
    }
    if (combo.extMaj7) {
        result |= 0b0000100000000000;  // Major 7th: 7(11)
    }
    if (combo.ext9) {
        result |= 0b0100000000000000;  // 9th: 9(14)
    }

    return result;
}

void ChordEffect::ReleaseAllChords(deque<MidiPacket>& output)
{
    // Go through all keys in noteOwner and send note off
    for (auto& pair : noteOwner) {
        uint8_t note = pair.first;
        output.push_back(MidiPacket::NoteOff(0, note, 0));
    }

    // Also need to send note off for root notes
    for (auto& pair : noteMap) {
        uint8_t root = pair.first;
        output.push_back(MidiPacket::NoteOff(0, root, 0));
    }

    // Clear all maps and vectors
    noteMap.clear();
    noteOwner.clear();
    noteOrder.clear();
}

void ChordEffect::UpdateChords(deque<MidiPacket>& output)
{
    // First, note off all current chord notes (not root notes)
    for (auto& pair : noteMap) {
        uint8_t root = pair.first;
        NoteData& data = pair.second;
        for (uint8_t chordNote : data.chordNotes) {
            output.push_back(MidiPacket::NoteOff(0, chordNote, 0));
        }
    }

    // Clear noteOwner for chord notes
    noteOwner.clear();

    // Recalculate and send new chords for all active root notes in FIFO order
    for (uint8_t root : noteOrder) {
        if (noteMap.find(root) == noteMap.end()) continue; // Safety check

        NoteData& data = noteMap[root];
        vector<uint8_t> newChordNotes;

        // Use the velocity stored in NoteData
        uint8_t velocity = data.velocity;

        // Calculate new chord notes based on updated chord bitmap
        for (uint8_t interval = 1; interval < 16; interval++) {
            if (chord & (1 << interval)) {
                uint8_t chordNote = root + interval;
                if (chordNote < 128) {
                    // Check if another root already owns this note
                    if (noteOwner.find(chordNote) != noteOwner.end()) {
                        // Find the previous owner and remove from their vector
                        uint8_t prevOwner = noteOwner[chordNote];
                        if (noteMap.find(prevOwner) != noteMap.end()) {
                            auto& prevData = noteMap[prevOwner];
                            prevData.chordNotes.erase(std::remove(prevData.chordNotes.begin(), prevData.chordNotes.end(), chordNote), prevData.chordNotes.end());
                        }
                    }

                    newChordNotes.push_back(chordNote);
                    noteOwner[chordNote] = root;
                    output.push_back(MidiPacket::NoteOn(0, chordNote, velocity)); // Use saved velocity
                }
            }
        }

        // Update the noteMap with new chord notes
        data.chordNotes = newChordNotes;
    }
}