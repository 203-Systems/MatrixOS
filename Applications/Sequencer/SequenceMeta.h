#pragma once

enum SequenceTrackMode {
   NoteTrack = 0x00,
   ControlChangeTrack = 0x20,
};

enum SequenceNoteType {
   Scale = 0x00,
   Chromatic = 0x01,
   Piano = 0x02,
   Drum = 0x04,
};

struct SequenceTrackModeConfig {
    union {
        struct {
            SequenceNoteType type:3;
            uint16_t scale:13;
            uint8_t root;
            uint8_t octave; // For UI
        } note;

        struct {
            uint8_t parameter;
        } param;
    };
};

struct SequenceMetaTrack {
    Color color;
    SequenceTrackMode mode;
    SequenceTrackModeConfig config;
};

struct SequenceMeta {
    std::string name;
    Color color;
    vector<SequenceMetaTrack> tracks;
};
