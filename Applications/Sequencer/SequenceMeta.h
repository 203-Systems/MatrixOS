#pragma once

#include "MatrixOS.h"
#include "Scales.h"

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
    std::string name;
    Color color;
    bool clockOutput;
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
        constexpr float hueStep = 1.0f / 16.0f;
        uint8_t hueIndex = MatrixOS::SYS::Millis() % 16;
        float hue = hueIndex * hueStep;
        color = Color::HsvToRgb(hue, 1.0f, 1.0f);
        clockOutput = false;
        for(uint8_t i = 0; i < tracks; i++)
        {
            SequenceMetaTrack track;
            track.color = colors[i];
            track.mode = SequenceTrackMode::NoteTrack;
            track.config.note.type = SequenceNoteType::Scale;
            track.config.note.customScale = false;
            track.config.note.enforceScale = true;
            track.config.note.scale = (uint16_t)Scale::MINOR;
            track.config.note.root = 0;
            track.config.note.rootOffset = 0;
            track.config.note.octave = 0;
            this->tracks.push_back(track);
        }
    }
};
