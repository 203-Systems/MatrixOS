#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

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
    vector<uint8_t>* noteSelected;
    std::function<void(uint8_t)> selectCallback;

    std::vector<uint8_t> noteMap;
    uint16_t c_aligned_scale_map;

    public:
    SequencerNotePad(Sequencer* sequencer, vector<uint8_t>* noteSelected);

    void OnSelect(std::function<void(uint8_t)> callback);

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

    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo);
    virtual bool Render(Point origin);
};
