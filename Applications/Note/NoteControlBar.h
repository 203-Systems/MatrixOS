#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "Note.h"
#include "NotePad.h"
#include "UnderglowLight.h"

#define CTL_BAR_Y 4

enum NoteControlBarMode : uint8_t {
  OFF_MODE,
  CHORD_MODE,
  ARP_MODE,
  KEY_MODE,
};

class NoteControlBar : public UIComponent {
  private:
    Note* note;
    NotePad* notePad[2];
    UnderglowLight* underglow[2];
    uint32_t shift[2];
    bool shift_event[2];
    static const uint32_t hold_threshold = 500; // Define hold threshold
    NoteControlBarMode mode = OFF_MODE;

    void SwapActiveConfig();
    bool ShiftActive();
    void ShiftEventOccured();
    void ShiftClear();
    Color GetOctavePlusColor();
    Color GetOctaveMinusColor();

    bool ChordControlKeyEvent(Point xy, KeyInfo* keyInfo);
    bool ArpControlKeyEvent(Point xy, KeyInfo* keyInfo);
    bool KeyControlKeyEvent(Point xy, KeyInfo* keyInfo);

    void RenderChordControl(Point origin);
    void RenderArpControl(Point origin);
    void RenderKeyControl(Point origin);
  public:
    NoteControlBar(Note* notePtr, NotePad* notepad1, NotePad* notepad2, UnderglowLight* underglow1, UnderglowLight* underglow2);

    virtual Dimension GetSize() override { return Dimension(8, CTL_BAR_Y); }
    virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) override;
    virtual bool Render(Point origin) override;
};