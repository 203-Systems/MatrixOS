#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"
#include "Scales.h"

enum class NoteType : uint8_t {
    ROOT_NOTE,
    SCALE_NOTE,
    OFF_SCALE_NOTE,
};

extern const Color sequencerPolyNoteColor[12];
extern const Color sequencerRainbowNoteColor[12];

class SequencerNotePad : public UIComponent {
    Sequencer* sequencer;

    bool rescanNeeded = false;
    bool prevTwoRowMode = false;
    bool testingMode = false;

    std::vector<uint8_t> noteMap;
    uint16_t c_aligned_scale_map;

    void SequencerEvent(const MidiPacket& packet);
    bool TwoRowMode();

    public:
    SequencerNotePad(Sequencer* sequencer, bool testingMode = false);

    Dimension GetSize();

    NoteType InScale(int16_t note);
    int16_t NoteFromRoot(int16_t note);
    int16_t GetNextInScaleNote(int16_t note);

    void GenerateOctaveKeymap();
    void GenerateChromaticKeymap();
    void GeneratePianoKeymap();
    void GenerateDrumKeymap();
    void GenerateKeymap();

    bool RenderRootNScale(Point origin);
    bool RenderPiano(Point origin);
    bool RenderDrum(Point origin);

    void Rescan(Point origin);

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool IsEnabled() override;
    virtual bool Render(Point origin);
};
