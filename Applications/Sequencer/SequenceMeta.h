#pragma once

#include "MatrixOS.h"
#include "Scales.h"

enum class SequenceTrackMode {
   NoteTrack = 0x00,
   ControlChangeTrack = 0x20,
};

enum class SequenceNoteType {
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
    
    void New(uint8_t tracks)
    {
        const Color colors[8]
        {
            Color(0x00FFFF),
            Color(0x0000FF),
            Color(0x8000FF),
            Color(0xFF00FF),
            Color(0xFF0080),
            Color(0xFF4000),
            Color(0xFFFF00),
            Color(0x00FF40)
        };


        this->tracks.reserve(tracks);
        color = Color(0xFF00FF); // TODO: Random Color
        for(uint8_t i = 0; i < tracks; i++)
        {
            SequenceMetaTrack track;
            track.color = colors[i];
            track.mode = SequenceTrackMode::NoteTrack;
            track.config.note.type = SequenceNoteType::Scale;
            track.config.note.scale = (uint16_t)Scale::MINOR;
            track.config.note.root = 0;
            track.config.note.octave = 0;
            this->tracks.push_back(track);
        }
    }
};
