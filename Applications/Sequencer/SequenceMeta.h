#pragma once

#include "MatrixOS.h"
#include "Scales.h"
#include <vector>
#include <cstdint>

enum class SequenceTrackMode {
   NoteTrack = 0x00,
   DrumTrack = 0x10,
   ControlChangeTrack = 0x20,
};

enum class SequenceNoteType {
   Scale = 0x00,
   Chromatic = 0x01,
   Piano = 0x02,
};

struct SequenceTrackModeConfig {
    struct {
        SequenceNoteType type:4;
        bool customScale:1;
        bool enforceScale:1;
        uint16_t scale;
        uint8_t root;
        uint8_t rootOffset;
        uint8_t octave; // For UI
    } note;

    struct {
        uint8_t parameter;
    } cc;
};

struct SequenceMetaTrack {
    Color color;
    bool velocitySensitive = true;
    SequenceTrackMode mode;
    SequenceTrackModeConfig config;
};

struct SequenceMeta {
    Color color;
    bool clockOutput;
    vector<SequenceMetaTrack> tracks;

    void New(uint8_t tracks);
};

// CBOR serialization helpers (streaming sequence)
bool SerializeSequenceMeta(const SequenceMeta& meta, File& file);
bool DeserializeSequenceMeta(File& file, SequenceMeta& out);
