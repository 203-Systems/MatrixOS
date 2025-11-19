#pragma once

#include "MatrixOS.h"
#include "SequenceEvent.h"

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

struct SequenceMeta {
    std::string name;
    Color color;
};

struct SequencePattern {
    uint8_t quarterNotes;
    std::map<uint16_t, SequenceEvent> events;

    void Clear()
    {
        events.clear();
    }
};


struct SequenceTrack {
    Color color;
    uint8_t channel;
    SequenceTrackMode mode;
    SequenceTrackModeConfig config;
    vector<SequencePattern> patterns;

    void Free()
    {
        for (auto& pattern : patterns) {
            pattern.Clear();
        }
        patterns.clear();
    }
};  

struct SequenceData {
    uint8_t version;
    uint16_t bpm;
    uint8_t swing;  
    vector<SequenceTrack> trackList;

    void Free()
    {
        for (auto& track : trackList) {
            track.Free();
        }
        trackList.clear();
    }
};
