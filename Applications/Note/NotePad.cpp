#include "NotePad.h"

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

NotePad::NotePad(Dimension dimension, NotePadData* data) {
    this->dimension = dimension;
    this->data = data;
    data->midiPipeline.AddEffect("NoteLatch", &data->noteLatch);
    data->noteLatch.SetEnabled(false);
    GenerateKeymap();
}

NotePad::~NotePad() {
    MatrixOS::MIDI::Send(MidiPacket::ControlChange(data->config->channel, 123, 0)); // All notes off
}

void NotePad::Tick()
{
    data->midiPipeline.Tick();
    MidiPacket midiPacket;
    while (data->midiPipeline.Get(midiPacket)) {
        // Update pipeline feedback notes based on Note On/Off events
        if (midiPacket.status == NoteOn && midiPacket.Velocity() > 0) {
            SetPipelineFeedbackNote(midiPacket.Note());
        }
        else if (midiPacket.status == NoteOff || (midiPacket.status == NoteOn && midiPacket.Velocity() == 0)) {
            UnsetPipelineFeedbackNote(midiPacket.Note());
        }

        MatrixOS::MIDI::Send(midiPacket);
    }
}

Color NotePad::GetColor() {
    return data->config->rootColor;
}

Dimension NotePad::GetSize() {
    return dimension;
}

NoteType NotePad::InScale(uint8_t note) {
    note %= 12;

    if (note == data->config->rootKey)
        return ROOT_NOTE;  // It is a root key
    return bitRead(c_aligned_scale_map, note) ? SCALE_NOTE : OFF_SCALE_NOTE;
}

uint8_t NotePad::NoteFromRoot(uint8_t note) {
    return (note + data->config->rootKey) % 12;
}

uint8_t NotePad::GetNextInScaleNote(uint8_t note) {
    for (int8_t i = 0; i < 12; i++) {
        note++;
        if (InScale(note) == SCALE_NOTE || InScale(note) == ROOT_NOTE) {
            return note;
        }
    }
    return UINT8_MAX;
}

uint8_t NotePad::GetActiveNoteCount(uint8_t note, bool upper) {
    if (note >= 128) return 0;
    uint8_t index = note / 2;
    if (upper) {
        return (activeNotes[index] >> 4) & 0x0F; // Upper nibble
    } else {
        return activeNotes[index] & 0x0F; // Lower nibble
    }
}

void NotePad::SetActiveNoteCount(uint8_t note, bool upper, uint8_t count) {
    if (note >= 128 || count > 15) return;
    uint8_t index = note / 2;
    if (upper) {
        activeNotes[index] = (activeNotes[index] & 0x0F) | ((count & 0x0F) << 4); // Set upper nibble
    } else {
        activeNotes[index] = (activeNotes[index] & 0xF0) | (count & 0x0F); // Set lower nibble
    }
}

bool NotePad::IsNoteActive(uint8_t note) {
    if (note >= 128) return false;
    bool upper = (note % 2) == 1;

    // Check active notes
    if (GetActiveNoteCount(note, upper) > 0) {
        return true;
    }

    // Check pipeline feedback notes
    return IsPipelineFeedbackNoteActive(note);
}

void NotePad::IncrementActiveNote(uint8_t note) {
    if (note >= 128) return;
    bool upper = (note % 2) == 1;
    uint8_t count = GetActiveNoteCount(note, upper);
    if (count < 15) {
        SetActiveNoteCount(note, upper, count + 1);
    }
}

void NotePad::DecrementActiveNote(uint8_t note) {
    if (note >= 128) return;
    bool upper = (note % 2) == 1;
    uint8_t count = GetActiveNoteCount(note, upper);
    if (count > 0) {
        SetActiveNoteCount(note, upper, count - 1);
    }
}

void NotePad::SetPipelineFeedbackNote(uint8_t note) {
    if (note >= 128) return;
    uint8_t byteIndex = note / 8;
    uint8_t bitIndex = note % 8;
    if (byteIndex < 16) {
        pipelineFeedbackNotes[byteIndex] |= (1 << bitIndex);
    }
}

void NotePad::UnsetPipelineFeedbackNote(uint8_t note) {
    if (note >= 128) return;
    uint8_t byteIndex = note / 8;
    uint8_t bitIndex = note % 8;
    if (byteIndex < 16) {
        pipelineFeedbackNotes[byteIndex] &= ~(1 << bitIndex);
    }
}

bool NotePad::IsPipelineFeedbackNoteActive(uint8_t note) {
    if (note >= 128) return false;
    uint8_t byteIndex = note / 8;
    uint8_t bitIndex = note % 8;
    if (byteIndex < 16) {
        return (pipelineFeedbackNotes[byteIndex] & (1 << bitIndex)) != 0;
    }
    return false;
}

void NotePad::GenerateOctaveKeymap() {
    noteMap.reserve(dimension.Area());
    uint8_t root = 12 * data->config->octave + data->config->rootKey;
    uint8_t nextNote = root;
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
            else if(!data->config->inKeyNoteOnly) { // If enforce scale is false, just add the next note
                noteMap[id] = nextNote;  // Add to map
                nextNote++;
            }
            else { // If enforce scale is true, find the next note that is in scale
                while (true) { // Find next key that we should put in
                    uint8_t inScale = InScale(nextNote);
                    if (inScale == ROOT_NOTE) { rootCount++; }
                    if (inScale == SCALE_NOTE || inScale == ROOT_NOTE) {
                        noteMap[id] = nextNote;  // Add to map
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
    uint8_t root = 12 * data->config->octave + data->config->rootKey;
    if (!data->config->inKeyNoteOnly) {
        for (int8_t y = 0; y < dimension.y; y++) {
            int8_t ui_y = dimension.y - y - 1;
            for (int8_t x = 0; x < dimension.x; x++) {
                uint8_t note = root + data->config->x_offset * x + data->config->y_offset * y;
                noteMap[ui_y * dimension.x + x] = note;
            }
        }
    }
    else {
        for (uint8_t y = 0; y < dimension.y; y++) {
            int8_t ui_y = dimension.y - y - 1;
            uint8_t note = root;

            for (uint8_t x = 0; x < dimension.x; x++) {
                noteMap[ui_y * dimension.x + x] = note;
                for (uint8_t i = 0; i < data->config->x_offset; i++) {
                    note = GetNextInScaleNote(note);
                }
            }

            for (uint8_t i = 0; i < data->config->y_offset; i++) {
                root = GetNextInScaleNote(root);
            }
        }
    }
}

void NotePad::GenerateChromaticKeymap() {
    noteMap.reserve(dimension.Area());
    uint8_t note = 12 * data->config->octave + data->config->rootKey;
    for(uint8_t i = 0; i < dimension.Area(); i++) {
        uint8_t x = i % dimension.x;
        uint8_t y = i / dimension.x;
        int8_t ui_y = dimension.y - y - 1;
        noteMap[ui_y * dimension.x + x] = note;
        if(!data->config->inKeyNoteOnly) {
            note++;
        }
        else {
            note = GetNextInScaleNote(note);
        }
    }
}

void NotePad::GeneratePianoKeymap() {
    noteMap.reserve(dimension.Area());
    const int8_t blackKeys[7] = {-1, 1,  3, -1, 6, 8, 10};
    const int8_t whiteKeys[7] = {0,  2,  4,  5, 7, 9, 11};

    for (int8_t y = 0; y < dimension.y; y++) {
        int8_t ui_y = dimension.y - y - 1;
        uint8_t octave = data->config->octave + (y / 2);

        if(y % 2 == 0) { // Bottom row
            for (int8_t x = 0; x < dimension.x; x++) {
                uint8_t id = ui_y * dimension.x + x;
                noteMap[id] = (octave + (x / 7)) * 12 + whiteKeys[x % 7];
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
                    noteMap[id] = (octave + (x / 7)) * 12 + offset;
                }
            }
        }
    }
}

void NotePad::GenerateKeymap() {
    c_aligned_scale_map = ((data->config->scale << data->config->rootKey) + ((data->config->scale & 0xFFF) >> (12 - data->config->rootKey % 12))) & 0xFFF;
    switch (data->config->mode) {
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
    memset(activeNotes, 0, sizeof(activeNotes)); // Initialize all counters to 0
    memset(pipelineFeedbackNotes, 0, sizeof(pipelineFeedbackNotes)); // Initialize pipeline feedback bitmap to 0
    data->midiPipeline.Send(MidiPacket::ControlChange(data->config->channel, 123, 0)); // All notes off
}

bool NotePad::RenderRootNScale(Point origin) {
    uint8_t index = 0;
    Color color_dim = data->config->useWhiteAsOutOfScale ? Color(0x202020) : data->config->color.Dim(32);
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
                uint8_t inScale = InScale(note);  // Check if the note is in scale.
                if (inScale == OFF_SCALE_NOTE) {
                    MatrixOS::LED::SetColor(globalPos, color_dim);
                }
                else if (inScale == SCALE_NOTE) {
                    MatrixOS::LED::SetColor(globalPos, data->config->color);
                }
                else if (inScale == ROOT_NOTE) {
                    MatrixOS::LED::SetColor(globalPos, data->config->rootColor);
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
    if(data->config->colorMode == COLOR_PER_KEY_POLY) {
        colorMap = polyNoteColor;
    }
    else if(data->config->colorMode == COLOR_PER_KEY_RAINBOW) {
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
                    MatrixOS::LED::SetColor(globalPos, data->config->useWhiteAsOutOfScale ? Color(0x202020) : Color(colorMap[awayFromRoot]).Dim(32));
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
    switch(data->config->colorMode) {
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
        data->midiPipeline.Send(MidiPacket::NoteOn(data->config->channel, note, data->config->forceSensitive ? keyInfo->Force().to7bits() : 0x7F));
        IncrementActiveNote(note);
    }
    else if (data->config->forceSensitive && keyInfo->State() == AFTERTOUCH) {
        data->midiPipeline.Send(MidiPacket::AfterTouch(data->config->channel, note, keyInfo->Force().to7bits()));
    }
    else if (keyInfo->State() == RELEASED) {
        data->midiPipeline.Send(MidiPacket::NoteOff(data->config->channel, note, 0));
        DecrementActiveNote(note);
    }
    return true;
}

void NotePad::SetDimension(Dimension dimension) {
    this->dimension = dimension;
    GenerateKeymap();
}

void NotePad::SetPadData(NotePadData* data) {
    this->data = data;
    GenerateKeymap();
}

