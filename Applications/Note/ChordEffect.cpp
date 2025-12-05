#include "ChordEffect.h"
#include <algorithm>

void ChordEffect::Tick(deque<MidiPacket>& input, deque<MidiPacket>& output) {
    if (disableOnNextTick) {
        disableOnNextTick = false;
        enabled = false;

        // Release all chords
        ReleaseAllChords(output);

        // Pass through remaining input
        for (const MidiPacket& packet : input) {
            output.push_back(packet);
        }
        return;
    }

    // If chord combo changed, update all active chords
    if (chordChanged) {
        UpdateChords(output);
        chordChanged = false;
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
    };
}

void ChordEffect::ProcessNoteOn(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t root = packet.Note();
    uint8_t velocity = packet.Velocity();
    uint8_t channel = packet.Channel();
    
    lastChannel = channel;

    // Initialize NoteData for this root note
    NoteData& data = noteMap[root];
    data.velocity = velocity;
    data.chordNotes.clear();

    // Add to note order if not already present
    auto it = std::find(noteOrder.begin(), noteOrder.end(), root);
    if (it == noteOrder.end()) {
        noteOrder.push_back(root);
    }

    // Build chord notes
    vector<uint8_t> newChordNotes = BuildChordFromNote(root);

    // Handle chord note conflicts and send MIDI
    for (uint8_t chordNote : newChordNotes) {
        // Check if another root note owns this chord note
        if (noteOwner.find(chordNote) != noteOwner.end()) {
            uint8_t previousOwner = noteOwner[chordNote];
            // Remove from previous owner's vector
            if (noteMap.find(previousOwner) != noteMap.end()) {
                auto& prevData = noteMap[previousOwner];
                prevData.chordNotes.erase(std::remove(prevData.chordNotes.begin(), prevData.chordNotes.end(), chordNote), prevData.chordNotes.end());
            }
        }

        // Add to current root's chord notes
        data.chordNotes.push_back(chordNote);
        // Update reverse lookup
        noteOwner[chordNote] = root;

        // Send note on
        output.push_back(MidiPacket::NoteOn(channel, chordNote, velocity));
    }
}

void ChordEffect::ProcessNoteOff(const MidiPacket& packet, deque<MidiPacket>& output) {
    uint8_t root = packet.Note();
    uint8_t channel = packet.Channel();

    // Check if this root note exists in noteMap
    if (noteMap.find(root) == noteMap.end()) return;

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

    // Send aftertouch for all chord notes in the noteMap under this root
    for (uint8_t chordNote : noteMap[root].chordNotes) {
        output.push_back(MidiPacket::AfterTouch(channel, chordNote, velocity));
    }
}

void ChordEffect::Reset() {
    noteMap.clear();
    noteOwner.clear();
    noteOrder.clear();
    chordChanged = true;
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

void ChordEffect::ClearChord() {
    chordCombo = {
        .dim = false,
        .min = false,
        .maj = false,
        .sus = false,
        .ext6 = false,
        .extMin7 = false,
        .extMaj7 = false,
        .ext9 = false
    };
    chordChanged = true;
}


void ChordEffect::SetChordCombo(ChordCombo combo) {
    if (chordCombo != combo) {
        chordCombo = combo;
        chordChanged = true;
    }
}

void ChordEffect::SetInversion(int8_t inversion)
{
    this->inversion = inversion;
    chordChanged = true;
}

void ChordEffect::CalculateChord() {
    // Clear and rebuild intervals directly
    chordIntervals.clear();
    chordIntervals.push_back(0);  // Root note

    // Base triads
    if (chordCombo.dim) {
        chordIntervals.push_back(3);  // b3
        chordIntervals.push_back(6);  // b5
    }

    if (chordCombo.min) {
        chordIntervals.push_back(3);  // b3
        chordIntervals.push_back(7);  // 5
    }

    if (chordCombo.maj) {
        chordIntervals.push_back(4);  // 3
        chordIntervals.push_back(7);  // 5
    }

    if (chordCombo.sus) {
        chordIntervals.push_back(5);  // 4
        chordIntervals.push_back(7);  // 5
    }

    // Extensions
    if (chordCombo.ext6) {
        chordIntervals.push_back(9);  // 6
    }
    if (chordCombo.extMin7) {
        chordIntervals.push_back(10); // b7
    }
    if (chordCombo.extMaj7) {
        chordIntervals.push_back(11); // 7
    }
    if (chordCombo.ext9) {
        chordIntervals.push_back(14); // 9
    }
}

void ChordEffect::ReleaseAllChords(deque<MidiPacket>& output)
{
    // Go through all keys in noteOwner and send note off
    for (auto& pair : noteOwner) {
        uint8_t note = pair.first;
        output.push_back(MidiPacket::NoteOff(lastChannel, note, 0));
    }
    
    // Clear all maps and vectors
    noteMap.clear();
    noteOwner.clear();
    noteOrder.clear();
}

void ChordEffect::UpdateChords(deque<MidiPacket>& output)
{
    CalculateChord();

    // Build new ownership map and chord notes for all roots
    unordered_map<uint8_t, uint8_t> newNoteOwner; // chordNote -> root
    unordered_map<uint8_t, vector<uint8_t>> newChordMap; // root -> new chord notes

    // Calculate new chords for all active roots in FIFO order
    for (uint8_t root : noteOrder) {
        if (noteMap.find(root) == noteMap.end()) continue;

        NoteData& data = noteMap[root];
        vector<uint8_t> newChordNotes = BuildChordFromNote(root);

        // Resolve conflicts - later roots (in FIFO) win
        for (uint8_t chordNote : newChordNotes) {
            if (newNoteOwner.find(chordNote) != newNoteOwner.end()) {
                // Remove from previous owner's chord
                uint8_t prevOwner = newNoteOwner[chordNote];
                auto& prevChord = newChordMap[prevOwner];
                prevChord.erase(std::remove(prevChord.begin(), prevChord.end(), chordNote), prevChord.end());
            }
            newNoteOwner[chordNote] = root;
        }

        newChordMap[root] = newChordNotes;
    }

    // Now compare old vs new and only send changes
    // 1. Find notes to turn off (in old but not in new, or changed owner)
    for (auto& chordNote : noteOwner) {
        uint8_t note = chordNote.first;
        uint8_t oldOwner = chordNote.second;

        // Check if note no longer exists or changed owner
        if (newNoteOwner.find(note) == newNoteOwner.end() || newNoteOwner[note] != oldOwner) {
            output.push_back(MidiPacket::NoteOff(lastChannel, note, 0));
        }
    }

    // 2. Find notes to turn on (in new but not in old, or changed owner)
    for (auto& chordNote : newNoteOwner) {
        uint8_t note = chordNote.first;
        uint8_t newOwner = chordNote.second;

        // Check if note is new or changed owner
        if (noteOwner.find(note) == noteOwner.end() || noteOwner[note] != newOwner) {
            uint8_t velocity = noteMap[newOwner].velocity;
            output.push_back(MidiPacket::NoteOn(lastChannel, note, velocity));
        }
    }

    // Update the state
    noteOwner = newNoteOwner;
    for (auto& pair : noteMap) {
        uint8_t root = pair.first;
        if (newChordMap.find(root) != newChordMap.end()) {
            pair.second.chordNotes = newChordMap[root];
        } else {
            pair.second.chordNotes.clear();
        }
    }
}

vector<uint8_t> ChordEffect::BuildChordFromNote(uint8_t root)
{
    vector<uint8_t> chordNotes;

    // Use pre-calculated intervals from CalculateChord
    const vector<uint8_t>& intervals = chordIntervals;

    // If no intervals or only root, just return root note
    if (intervals.size() <= 1) {
        chordNotes.push_back(root);
        return chordNotes;
    }

    // Apply inversion logic
    for (uint8_t i = 0; i < intervals.size(); i++) {
        uint8_t interval = intervals[i];
        uint8_t chordNote = root + interval;

        // Apply inversion: notes below the inversion point get moved up by octaves
        if (i < (inversion % intervals.size())) {
            uint8_t octaveShift = (inversion / intervals.size() + 1) * 12;
            chordNote += octaveShift;
        } else if (inversion >= intervals.size()) {
            // For higher inversions, add base octave shifts
            uint8_t octaveShift = (inversion / intervals.size()) * 12;
            chordNote += octaveShift;
        }

        if (chordNote < 128) {
            chordNotes.push_back(chordNote);
        }
    }

    return chordNotes;
}