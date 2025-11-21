#include "NotePad.h"

SequencerNotePad::SequencerNotePad(Sequencer* sequencer)
{
    this->sequencer = sequencer;
    prevPatternView = sequencer->patternViewActive;
    GenerateKeymap();
}

void SequencerNotePad::OnSelect(std::function<void(bool, uint8_t, uint8_t)> callback)
{
    selectCallback = callback;
}

Dimension SequencerNotePad::GetSize()
{
    return Dimension(8, 4);
}

NoteType SequencerNotePad::InScale(int16_t note)
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];

    note += 120; // add a big pos offset to make sure we don't get negative after mod
    note %= 12;

    if (note == (metaTrack.config.note.root % 12))
        return NoteType::ROOT_NOTE;  // It is a root key
    return bitRead(c_aligned_scale_map, note) ? NoteType::SCALE_NOTE : NoteType::OFF_SCALE_NOTE;
}

int16_t SequencerNotePad::NoteFromRoot(int16_t note)
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];
    return (note - metaTrack.config.note.root + 24) % 12;
}

int16_t SequencerNotePad::GetNextInScaleNote(int16_t note)
{
    for (int8_t i = 0; i < 12; i++) {
        note++;
        if (InScale(note) == NoteType::SCALE_NOTE || InScale(note) == NoteType::ROOT_NOTE) {
            return note;
        }
    }
    return INT16_MAX;
}

void SequencerNotePad::GenerateOctaveKeymap()
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];
    Dimension dimension = GetSize();

    noteMap.clear();
    noteMap.reserve(dimension.Area());

    int16_t root = 12 * metaTrack.config.note.octave + metaTrack.config.note.root;
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
            else if(!metaTrack.config.note.enforceScale) { // If enforce scale is false, just add the next note
                noteMap[id] = nextNote;  // Add to map
                nextNote++;
            }
            else { // Find the next note that is in scale
                while (true) { // Find next key that we should put in
                    NoteType inScale = InScale(nextNote);
                    if (inScale == NoteType::ROOT_NOTE) { rootCount++; }
                    if (inScale == NoteType::SCALE_NOTE || inScale == NoteType::ROOT_NOTE) {
                        noteMap[id] = nextNote < 0 ? 255 : (uint8_t)nextNote;  // Add to map
                        nextNote++;
                        break;  // Break from inf loop
                    }
                    else if (inScale == NoteType::OFF_SCALE_NOTE) {
                        nextNote++;
                        continue;  // Check next note
                    }
                }
            }
        }
    }
}

void SequencerNotePad::GenerateChromaticKeymap()
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];
    Dimension dimension = GetSize();

    noteMap.clear();
    noteMap.reserve(dimension.Area());

    int16_t note = (12 * metaTrack.config.note.octave) + metaTrack.config.note.root;
    for(uint8_t i = 0; i < dimension.Area(); i++) {
        uint8_t x = i % dimension.x;
        uint8_t y = i / dimension.x;
        int8_t ui_y = dimension.y - y - 1;
        if (note > 127 || note < 0) {
            noteMap[ui_y * dimension.x + x] = 255;
        } else {
            noteMap[ui_y * dimension.x + x] = note;
        }
        if(!metaTrack.config.note.enforceScale) {
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

void SequencerNotePad::GeneratePianoKeymap()
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];
    Dimension dimension = GetSize();

    noteMap.clear();
    noteMap.reserve(dimension.Area());

    const int8_t blackKeys[7] = {-1, 1,  3, -1, 6, 8, 10};
    const int8_t whiteKeys[7] = {0,  2,  4,  5, 7, 9, 11};

    for (int8_t y = 0; y < dimension.y; y++) {
        int8_t ui_y = dimension.y - y - 1;
        int16_t octave = metaTrack.config.note.octave + (y / 2);

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

void SequencerNotePad::GenerateDrumKeymap()
{
    Dimension dimension = GetSize();
    noteMap.clear();
    noteMap.reserve(dimension.Area());

    // Placeholder: Fill with 255
    for(uint8_t i = 0; i < dimension.Area(); i++) {
        uint8_t x = i % dimension.x;
        uint8_t y = i / dimension.x;
        uint8_t ui_y = dimension.y - y - 1;

        uint8_t note = 0;
        if(sequencer->patternViewActive)
        {   
            note = (x % 4) + ui_y * 4;
            if(x >= 4) {note += 8;}
            if(ui_y >= 2) {note = 255;}
        }
        else
        {
            note = (x % 4) + ui_y * 4;
            if(x >= 4) {note += 16;}
        }
        noteMap[i] = note;
    }
}

void SequencerNotePad::GenerateKeymap()
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];

    if(metaTrack.mode != SequenceTrackMode::NoteTrack && metaTrack.mode != SequenceTrackMode::DrumTrack)
    {
        noteMap.resize(GetSize().Area(), 255);
        return;
    }

    if(metaTrack.mode == SequenceTrackMode::NoteTrack)
    {
        c_aligned_scale_map = (((uint16_t)metaTrack.config.note.scale << metaTrack.config.note.root) +
                        (((uint16_t)metaTrack.config.note.scale & 0xFFF) >> (12 - metaTrack.config.note.root % 12))) & 0xFFF;

        switch (metaTrack.config.note.type) {
            case SequenceNoteType::Scale:
                GenerateOctaveKeymap();
                break;
            case SequenceNoteType::Chromatic:
                GenerateChromaticKeymap();
                break;
            case SequenceNoteType::Piano:
                GeneratePianoKeymap();
                break;
        }
    }
    else if(metaTrack.mode == SequenceTrackMode::DrumTrack)
    {
        GenerateDrumKeymap();
        return;
    }

    // Turn off all active keys
    uint8_t channel = sequencer->sequence.GetChannel(track);
    for (const auto& [note, velocity] : sequencer->noteSelected) {
        MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0));
    }

    sequencer->noteSelected.clear();

    rescanNeeded = true;
}

bool SequencerNotePad::KeyEvent(Point xy, KeyInfo* keyInfo)
{
    uint8_t note = noteMap[xy.y * 8 + xy.x];
    if (note == 255) {
        return true;
    }

    uint8_t track = sequencer->track;
    uint8_t channel = sequencer->sequence.GetChannel(track);

    if(keyInfo->state == PRESSED)
    {
        uint8_t velocity = 127;
        if(sequencer->meta.tracks[track].velocitySensitive)
        {
            velocity = keyInfo->Value().to7bits();
        }
        sequencer->noteSelected[note] = velocity;
        if(selectCallback != nullptr)
        {
            selectCallback(true, note, velocity);
        }
        MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, note, velocity));
    }
    else if(keyInfo->state == AFTERTOUCH && sequencer->meta.tracks[track].velocitySensitive)
    {
        if(sequencer->noteSelected.count(note) != 0) // Incase we need to do first scan first
        {
            uint8_t velocity = keyInfo->Value().to7bits();
            sequencer->noteSelected[note] = velocity;
            selectCallback(false, note, velocity);
            MatrixOS::MIDI::Send(MidiPacket::AfterTouch(channel, note, velocity));
        }
    }
    else if(keyInfo->state == RELEASED)
    {
        // Remove note from selection
        sequencer->noteSelected.erase(note);
        MatrixOS::MIDI::Send(MidiPacket::NoteOff(channel, note, 0));
        selectCallback(false, note, 0);
    }

    return true;
}

bool SequencerNotePad::RenderRootNScale(Point origin)
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];
    Dimension dimension = GetSize();
    Color rootColor = metaTrack.color;
    Color scaleColor = Color(0x808080);
    Color offScaleColor = Color(0x202020);

    uint8_t index = 0;
    for (int8_t y = 0; y < dimension.y; y++) {
        for (int8_t x = 0; x < dimension.x; x++) {
            uint8_t note = noteMap[index];
            Point globalPos = origin + Point(x, y);

            if (note == 255) {
                MatrixOS::LED::SetColor(globalPos, Color(0));
            }
            else {
                // Check if note is selected or active
                bool isSelected = sequencer->noteSelected.count(note) > 0;
                bool isActive = sequencer->noteActive.count(note) > 0;

                if (isSelected) {
                    MatrixOS::LED::SetColor(globalPos, Color::White);
                }
                else if (isActive) {
                    MatrixOS::LED::SetColor(globalPos, Color(0x00FF00)); // Green
                }
                else {
                    NoteType inScale = InScale(note);
                    if (inScale == NoteType::OFF_SCALE_NOTE) {
                        MatrixOS::LED::SetColor(globalPos, offScaleColor);
                    }
                    else if (inScale == NoteType::SCALE_NOTE) {
                        MatrixOS::LED::SetColor(globalPos, scaleColor);
                    }
                    else if (inScale == NoteType::ROOT_NOTE) {
                        MatrixOS::LED::SetColor(globalPos, rootColor);
                    }
                }
            }
            index++;
        }
    }
    return true;
}

bool SequencerNotePad::RenderPiano(Point origin)
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];
    Dimension dimension = GetSize();

    uint8_t index = 0;
    for (int8_t y = 0; y < dimension.y; y++) {
        int8_t ui_y = dimension.y - y - 1;
        bool isBlackKeyRow = (ui_y % 2 == 1);

        for (int8_t x = 0; x < dimension.x; x++) {
            uint8_t note = noteMap[index];
            Point globalPos = origin + Point(x, y);

            if (note == 255) {
                MatrixOS::LED::SetColor(globalPos, Color(0));
            }
            else {
                // Check if note is selected or active
                bool isSelected = sequencer->noteSelected.count(note) > 0;
                bool isActive = sequencer->noteActive.count(note) > 0;

                if (isSelected) {
                    MatrixOS::LED::SetColor(globalPos, Color::White);
                }
                else if (isActive) {
                    MatrixOS::LED::SetColor(globalPos, Color(0x00FF00)); // Green
                }
                else {
                    // Check if note is root, in scale, or out of scale
                    NoteType inScale = InScale(note);
                    if (inScale == NoteType::ROOT_NOTE) {
                        // Root key: track color
                        MatrixOS::LED::SetColor(globalPos, metaTrack.color);
                    }
                    else if (inScale == NoteType::SCALE_NOTE) {
                        // In scale key: gray
                        MatrixOS::LED::SetColor(globalPos, Color(0x808080));
                    }
                    else {
                        // Out of scale key: darker gray
                        MatrixOS::LED::SetColor(globalPos, Color(0x202020));
                    }
                }
            }
            index++;
        }
    }
    return true;
}

bool SequencerNotePad::RenderDrum(Point origin)
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];
    Dimension dimension = GetSize();
    Color drumColor = metaTrack.color;

    uint8_t index = sequencer->patternViewActive ? 16 : 0;
    for (int8_t y = sequencer->patternViewActive ? 2 : 0; y < dimension.y; y++) {
        for (int8_t x =  0; x < dimension.x; x++) {
            uint8_t note = noteMap[index];
            Point globalPos = origin + Point(x, y);

            if (note == 255) {
                MatrixOS::LED::SetColor(globalPos, Color(0));
            }
            else {
                // Check if note is selected or active
                bool isSelected = sequencer->noteSelected.count(note) > 0;
                bool isActive = sequencer->noteActive.count(note) > 0;
                
                if (isSelected) {
                    MatrixOS::LED::SetColor(globalPos, Color::White);
                }
                else if (isActive) {
                    MatrixOS::LED::SetColor(globalPos, Color(0x00FF00)); // Green
                }
                else {
                    MatrixOS::LED::SetColor(globalPos, note < 16 ? drumColor : Color(0x808080));
                }
            }
            index++;
        }
    }
    return true;
}

void SequencerNotePad::Rescan(Point origin)
{
    uint8_t track = sequencer->track;
    uint8_t channel = sequencer->sequence.GetChannel(track);

    for(uint8_t y = 0; y < GetSize().y; y++)
    {
        for(uint8_t x = 0; x < GetSize().x; x++)
        {
            Point pos = origin + Point(x, y);
            KeyInfo* keyInfo = MatrixOS::KeyPad::GetKey(pos);

            if(keyInfo->Active())
            {
                uint8_t note = noteMap[y * 8 + x];
                if(note != 255)
                {
                    uint8_t velocity = 127;
                    if(sequencer->meta.tracks[track].velocitySensitive)
                    {
                        velocity = keyInfo->Value().to7bits();
                    }
                    sequencer->noteSelected[note] = velocity;
                    if(selectCallback != nullptr)
                    {
                        selectCallback(true, note, velocity);
                    }
                    MatrixOS::MIDI::Send(MidiPacket::NoteOn(channel, note, velocity));
                }
            }
        }
    }
    rescanNeeded = false;
}

bool SequencerNotePad::Render(Point origin)
{
    uint8_t track = sequencer->track;
    SequenceMetaTrack& metaTrack = sequencer->meta.tracks[track];

    if(sequencer->patternViewActive != prevPatternView)
    {
        prevPatternView = sequencer->patternViewActive;
        GenerateKeymap();
    }

    if(metaTrack.mode != SequenceTrackMode::NoteTrack && metaTrack.mode != SequenceTrackMode::DrumTrack)
    {
        return false;
    }

    if(rescanNeeded)
    {
        Rescan(origin);
    }

    if(metaTrack.mode == SequenceTrackMode::NoteTrack)
    {
        switch (metaTrack.config.note.type) {
            case SequenceNoteType::Scale:
            case SequenceNoteType::Chromatic:
                return RenderRootNScale(origin);
            case SequenceNoteType::Piano:
                return RenderPiano(origin);
            default:
                return false;
        }
    }
    else if(metaTrack.mode == SequenceTrackMode::DrumTrack)
    {
        return RenderDrum(origin);
    }
    return false;
}
