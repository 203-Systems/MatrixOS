#include "NotePad.h"
#include <algorithm>

const Color polyNoteColor[12] = {
    Color(0x00FFD9),
    Color(0xFF0097),
    Color(0xFFFB00),
    Color(0x5D00FF),
    Color(0xFF4B00),
    Color(0x009BFF),
    Color(0xFF003E),
    Color(0xAEFF00),
    Color(0xED00FF),
    Color(0xFFAE00),
    Color(0x1000FF),
    Color(0xFF1D00)
};

const Color rainbowNoteColor[12] = {
    Color(0xFF0000), // Red
    Color(0xFF4000),
    Color(0xFFFF00), // Yellow
    Color(0x80FF00),
    Color(0x00FF00), // Green
    Color(0x00FF80),
    Color(0x00FFFF), // Cyan
    Color(0x0080FF),
    Color(0x0000FF), // Blue
    Color(0x8000FF),
    Color(0xFF00FF), // Magenta
    Color(0xFF0080)
};

NotePad::NotePad(Dimension dimension, NotePadRuntime* data) {
    this->dimension = dimension;
    this->rt = data;

    // Assign arpConfig pointer to point to the config's arpConfig
    rt->arpConfig = &rt->config->arpConfig;
    rt->arpeggiator = Arpeggiator(rt->arpConfig);

    rt->midiPipeline.AddEffect("NoteLatch", &rt->noteLatch);
    rt->noteLatch.SetEnabled(false);
    rt->midiPipeline.AddEffect("ChordEffect", &rt->chordEffect);
    rt->midiPipeline.AddEffect("Arpeggiator", &rt->arpeggiator);
    activeKeys.clear();
    GenerateKeymap();
}

NotePad::~NotePad() {
    MatrixOS::MIDI::Send(MidiPacket::ControlChange(rt->config->channel, 123, 0)); // All notes off
    activeKeys.clear();
}

void NotePad::Tick()
{
    rt->midiPipeline.Tick();
    MidiPacket midiPacket;
    while (rt->midiPipeline.Get(midiPacket)) {
        MatrixOS::MIDI::Send(midiPacket, MIDI_PORT_ALL);
    }
}

Color NotePad::GetColor() {
    return rt->config->rootColor;
}

Dimension NotePad::GetSize() {
    return dimension;
}

NoteType NotePad::InScale(int16_t note) {
    note %= 12;
    note = abs(note);

    if (note == rt->config->rootKey)
        return ROOT_NOTE;  // It is a root key
    return bitRead(c_aligned_scale_map, note) ? SCALE_NOTE : OFF_SCALE_NOTE;
}

int16_t NotePad::NoteFromRoot(int16_t note) {
    return (note + rt->config->rootKey) % 12;
}

int16_t NotePad::GetNextInScaleNote(int16_t note) {
    for (int8_t i = 0; i < 12; i++) {
        note++;
        if (InScale(note) == SCALE_NOTE || InScale(note) == ROOT_NOTE) {
            return note;
        }
    }
    return UINT8_MAX;
}

uint8_t NotePad::GetActiveNoteCount(uint8_t note) {
    if (note >= 128) return 0;
    bool upper = (note % 2) == 1;
    uint8_t index = note / 2;
    if (upper) {
        return (rt->activeNotes[index] >> 4) & 0x0F; // Upper nibble
    } else {
        return rt->activeNotes[index] & 0x0F; // Lower nibble
    }
}

void NotePad::SetActiveNoteCount(uint8_t note, uint8_t count) {
    if (note >= 128 || count > 15) return;
    bool upper = (note % 2) == 1;
    uint8_t index = note / 2;
    if (upper) {
        rt->activeNotes[index] = (rt->activeNotes[index] & 0x0F) | ((count & 0x0F) << 4); // Set upper nibble
    } else {
        rt->activeNotes[index] = (rt->activeNotes[index] & 0xF0) | (count & 0x0F); // Set lower nibble
    }
}

bool NotePad::IsNoteActive(uint8_t note) {
    if (note >= 128) return false;

    return (GetActiveNoteCount(note) > 0);
}

void NotePad::IncrementActiveNote(uint8_t note) {
    if (note >= 128) return;
    uint8_t count = GetActiveNoteCount(note);
    if (count < 15) {
        SetActiveNoteCount(note, count + 1);
    }
}

void NotePad::DecrementActiveNote(uint8_t note) {
    if (note >= 128) return;
    uint8_t count = GetActiveNoteCount(note);
    if (count > 0) {
        SetActiveNoteCount(note, count - 1);
    }
}

void NotePad::AddActiveKey(Point position, Fract16 velocity) {
    activeKeys.emplace_back(position, velocity);
}

void NotePad::RemoveActiveKey(Point position) {
    activeKeys.erase(
        std::remove_if(activeKeys.begin(), activeKeys.end(),
            [position](const ActiveKey& key) { return key.position == position; }),
        activeKeys.end()
    );
}

void NotePad::UpdateActiveKeyVelocity(Point position, Fract16 velocity) {
    for (auto& key : activeKeys) {
        if (key.position == position) {
            key.velocity = velocity;
            break;
        }
    }
}

void NotePad::GenerateOctaveKeymap() {
    noteMap.reserve(dimension.Area());
    int16_t root = 12 * rt->config->octave + rt->config->rootKey;
    int16_t nextNote = root;
    uint8_t rootCount = 0;
    for (int8_t y = 0; y < dimension.y; y++) {
        int8_t ui_y = dimension.y - y - 1;

        if (rootCount >= 2) {
            root += 12;
            rootCount = 0;
            nextNote = root;
        }

        for (int8_t x = 0; x < dimension.x; x++) {
            uint8_t id = ui_y * dimension.x + x;
            if (nextNote > 127) { // If next note is out of range, fill with 255
                noteMap[id] = 255;
            }
            else if(!rt->config->enforceScale) { // If enforce scale is false, just add the next note
                noteMap[id] = nextNote;  // Add to map
                nextNote++;
            }
            else { // If enforce scale is true, find the next note that is in scale
                while (true) { // Find next key that we should put in
                    uint8_t inScale = InScale(nextNote);
                    if (inScale == ROOT_NOTE) { rootCount++; }
                    if (inScale == SCALE_NOTE || inScale == ROOT_NOTE) {
                        noteMap[id] = nextNote < 0 ? 255 : (uint8_t)nextNote;  // Add to map
                        nextNote++;
                        break;  // Break from inf loop
                    }
                    else if (inScale == OFF_SCALE_NOTE) {
                        nextNote++;
                        continue;  // Check next note
                    }
                }
            }
        }
    }
}

void NotePad::GenerateOffsetKeymap() {
    noteMap.reserve(dimension.Area());
    int16_t root = 12 * rt->config->octave + rt->config->rootKey;
    if (!rt->config->enforceScale) {
        for (int8_t y = 0; y < dimension.y; y++) {
            int8_t ui_y = dimension.y - y - 1;
            for (int8_t x = 0; x < dimension.x; x++) {
                int16_t note = root + rt->config->x_offset * x + rt->config->y_offset * y;
                if (note > 127 || note < 0) {
                    noteMap[ui_y * dimension.x + x] = 255;
                } else {
                    noteMap[ui_y * dimension.x + x] = note;
                }
            }
        }
    }
    else {
        for (uint8_t y = 0; y < dimension.y; y++) {
            int8_t ui_y = dimension.y - y - 1;
            int16_t note = root;

            for (uint8_t x = 0; x < dimension.x; x++) {
                if (note > 127 || note < 0) {
                    noteMap[ui_y * dimension.x + x] = 255;
                } else {
                    noteMap[ui_y * dimension.x + x] = note;
                }
                for (uint8_t i = 0; i < rt->config->x_offset; i++) {
                    int16_t nextNote = GetNextInScaleNote(note);
                    if (nextNote == INT16_MAX) {
                        note = INT16_MAX; // Mark as invalid
                        break;
                    }
                    note = nextNote;
                }
            }

            for (uint8_t i = 0; i < rt->config->y_offset; i++) {
                int16_t nextRoot = GetNextInScaleNote(root);
                if (nextRoot == INT16_MAX) {
                    root = INT16_MAX; // Mark as invalid
                    break;
                }
                root = nextRoot;
            }
        }
    }
}

void NotePad::GenerateChromaticKeymap() {
    noteMap.reserve(dimension.Area());
    int16_t note = (12 * rt->config->octave) + rt->config->rootKey;
    for(uint8_t i = 0; i < dimension.Area(); i++) {
        uint8_t x = i % dimension.x;
        uint8_t y = i / dimension.x;
        int8_t ui_y = dimension.y - y - 1;
        if (note > 127 || note < 0) {
            noteMap[ui_y * dimension.x + x] = 255;
        } else {
            noteMap[ui_y * dimension.x + x] = note;
        }
        if(!rt->config->enforceScale) {
            note++;
        }
        else {
            int16_t nextNote = GetNextInScaleNote(note);
            if (nextNote == INT16_MAX) {
                note = INT16_MAX; // Mark as invalid, subsequent notes will be 255
            } else {
                note = nextNote;
            }
        }
    }
}

void NotePad::GeneratePianoKeymap() {
    noteMap.reserve(dimension.Area());
    const int8_t blackKeys[7] = {-1, 1,  3, -1, 6, 8, 10};
    const int8_t whiteKeys[7] = {0,  2,  4,  5, 7, 9, 11};

    for (int8_t y = 0; y < dimension.y; y++) {
        int8_t ui_y = dimension.y - y - 1;
        int16_t octave = rt->config->octave + (y / 2);

        if(y % 2 == 0) { // Bottom row
            for (int8_t x = 0; x < dimension.x; x++) {
                uint8_t id = ui_y * dimension.x + x;
                int16_t note = (octave + (x / 7)) * 12 + whiteKeys[x % 7];
                if (note > 127 || note < 0) {
                    noteMap[id] = 255;
                } else {
                    noteMap[id] = note;
                }
            }
        }
        else { // Top row
            for (int8_t x = 0; x < dimension.x; x++) {
                uint8_t id = ui_y * dimension.x + x;
                int8_t offset = blackKeys[x % 7];
                if(offset == -1) {
                    noteMap[id] = 255;
                }
                else {
                    int16_t note = (octave + (x / 7)) * 12 + offset;
                    if (note > 127 || note < 0) {
                        noteMap[id] = 255;
                    } else {
                        noteMap[id] = note;
                    }
                }
            }
        }
    }
}

void NotePad::GenerateKeymap() {
    c_aligned_scale_map = ((rt->config->scale << rt->config->rootKey) + ((rt->config->scale & 0xFFF) >> (12 - rt->config->rootKey % 12))) & 0xFFF;
    switch (rt->config->mode) {
        case OCTAVE_LAYOUT:
            GenerateOctaveKeymap();
            break;
        case OFFSET_LAYOUT:
            GenerateOffsetKeymap();
            break;
        case CHROMATIC_LAYOUT:
            GenerateChromaticKeymap();
            break;
        case PIANO_LAYOUT:
            GeneratePianoKeymap();
            break;
    }
    // Send NoteOff for all active notes before clearing
    for (uint8_t note = 0; note < 128; note++) {
        if (GetActiveNoteCount(note) > 0) {
            rt->midiPipeline.Send(MidiPacket::NoteOff(rt->config->channel, note, 0));
        }
    }
    memset(rt->activeNotes, 0, sizeof(rt->activeNotes)); // Initialize all counters to 0
    first_scan = true;

}

void NotePad::FirstScan(Point origin)
{
    for(const auto& activeKey : activeKeys)
    {
        uint8_t note = noteMap[activeKey.position.y * dimension.x + activeKey.position.x];

        if(note == 255) {
            continue;
        }

        IncrementActiveNote(note);
        Fract16 velocity = activeKey.velocity;
        rt->midiPipeline.Send(MidiPacket::NoteOn(rt->config->channel, note, velocity.to7bits()));
    }
}

bool NotePad::RenderRootNScale(Point origin) {
    uint8_t index = 0;
    Color color_dim = rt->config->useWhiteAsOutOfScale ? Color(0x202020) : rt->config->color.Dim(32);
    for (int8_t y = 0; y < dimension.y; y++) {
        for (int8_t x = 0; x < dimension.x; x++) {
            uint8_t note = noteMap[index];
            Point globalPos = origin + Point(x, y);
            if (note == 255) {
                MatrixOS::LED::SetColor(globalPos, Color(0));
            }
            else if (IsNoteActive(note) || rt->midiPipeline.IsNoteActive(note)) { // If find the note is currently active. Show it as white
                MatrixOS::LED::SetColor(globalPos, Color(0xFFFFFF));
            }
            else {
                uint8_t inScale = InScale(note);  // Check if the note is in scale.
                if (inScale == OFF_SCALE_NOTE) {
                    MatrixOS::LED::SetColor(globalPos, color_dim);
                }
                else if (inScale == SCALE_NOTE) {
                    MatrixOS::LED::SetColor(globalPos, rt->config->color);
                }
                else if (inScale == ROOT_NOTE) {
                    MatrixOS::LED::SetColor(globalPos, rt->config->rootColor);
                }
            }
            index++;
        }
    }
    return true;
}

bool NotePad::RenderColorPerKey(Point origin) {
    uint8_t index = 0;
    const Color* colorMap;
    if(rt->config->colorMode == COLOR_PER_KEY_POLY) {
        colorMap = polyNoteColor;
    }
    else if(rt->config->colorMode == COLOR_PER_KEY_RAINBOW) {
        colorMap = rainbowNoteColor;
    }
    else {
        return false;
    }

    for (int8_t y = 0; y < dimension.y; y++) {
        for (int8_t x = 0; x < dimension.x; x++) {
            uint8_t note = noteMap[index];
            Point globalPos = origin + Point(x, y);
            if (note == 255) {
                MatrixOS::LED::SetColor(globalPos, Color(0));
            }
            else if (IsNoteActive(note)) { // If find the note is currently active. Show it as white
                MatrixOS::LED::SetColor(globalPos, Color(0xFFFFFF));
            }
            else {
                uint8_t awayFromRoot = NoteFromRoot(note);
                uint8_t inScale = InScale(note);
                if (inScale == OFF_SCALE_NOTE) {
                    MatrixOS::LED::SetColor(globalPos, rt->config->useWhiteAsOutOfScale ? Color(0x202020) : Color(colorMap[awayFromRoot]).Dim(32));
                }
                else {
                    MatrixOS::LED::SetColor(globalPos, colorMap[awayFromRoot]);
                }
            }
            index++;
        }
    }
    return true;
}

bool NotePad::Render(Point origin) {
    if(first_scan) {
        FirstScan(origin);
        first_scan = false;
    }
    switch(rt->config->colorMode) {
        case ROOT_N_SCALE:
            return RenderRootNScale(origin);
            break;
        case COLOR_PER_KEY_POLY:
        case COLOR_PER_KEY_RAINBOW:
            return RenderColorPerKey(origin);
            break;
        default:
            return false;
    }
    return false;
}

bool NotePad::KeyEvent(Point xy, KeyInfo* keyInfo) {
    uint8_t note = noteMap[xy.y * dimension.x + xy.x];
    if (note == 255) {
        return true;
    }
    if (keyInfo->State() == PRESSED) {
        Fract16 velocity = rt->config->forceSensitive ? keyInfo->Force() : Fract16(0x7F << 9);
        rt->midiPipeline.Send(MidiPacket::NoteOn(rt->config->channel, note, velocity.to7bits()));
        AddActiveKey(xy, velocity);
        IncrementActiveNote(note);
    }
    else if (rt->config->forceSensitive && keyInfo->State() == AFTERTOUCH) {
        Fract16 velocity = keyInfo->Force();
        rt->midiPipeline.Send(MidiPacket::AfterTouch(rt->config->channel, note, velocity.to7bits()));
        UpdateActiveKeyVelocity(xy, velocity);
    }
    else if (keyInfo->State() == RELEASED) {
        rt->midiPipeline.Send(MidiPacket::NoteOff(rt->config->channel, note, 0));
        RemoveActiveKey(xy);
        DecrementActiveNote(note);
    }
    return true;
}

void NotePad::SetDimension(Dimension dimension) {
    this->dimension = dimension;
    GenerateKeymap();
}

void NotePad::SetPadRuntime(NotePadRuntime* rt) {
    this->rt = rt;
    GenerateKeymap();
}

