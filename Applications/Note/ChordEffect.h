#pragma once

#include "MatrixOS.h"
#include "MidiEffect.h"

struct NoteData {
    uint8_t velocity;
    vector<uint8_t> chordNotes;
};

struct ChordCombo {
    bool dim:1;
    bool min:1;
    bool maj:1;
    bool sus:1;

    bool ext6:1;
    bool extMin7:1;
    bool extMaj7:1;
    bool ext9:1;
};

class ChordEffect : public MidiEffect {
private:
    unordered_map<uint8_t, NoteData> noteMap;           // Maps root note -> its velocity and generated chord notes
    unordered_map<uint8_t, uint8_t> noteOwner;          // Reverse lookup: maps chord note -> root note that owns it
    vector<uint8_t> noteOrder;                          // Tracks insertion order for FIFO processing
    vector<uint8_t> chordIntervals;                     // Pre-calculated chord intervals
    bool chordChanged = true;
    bool disableOnNextTick = false;

    // Helper functions
    void ProcessNoteOn(const MidiPacket& packet, deque<MidiPacket>& output);
    void ProcessNoteOff(const MidiPacket& packet, deque<MidiPacket>& output);
    void ProcessAfterTouch(const MidiPacket& packet, deque<MidiPacket>& output);
    vector<uint8_t> BuildChordFromNote(uint8_t root);
    void CalculateChord();

public:
    int8_t inversion = 0;
    ChordCombo chordCombo = {0};
    
    void Tick(deque<MidiPacket>& input, deque<MidiPacket>& output) override;
    void Reset() override;
    void SetEnabled(bool state) override;
    void SetChordCombo(ChordCombo combo);
    void ReleaseAllChords(deque<MidiPacket>& output);
    void UpdateChords(deque<MidiPacket>& output);
    void SetInversion(int8_t inversion);
};