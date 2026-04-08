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
  static const uint32_t holdThreshold = 500; // Define hold threshold
  NoteControlBarMode mode = OFF_MODE;
  bool keyOffsetMode = false;
  uint32_t pitch_down = 0;
  uint32_t pitch_up = 0;
  bool chordExtKeyOn[4] = {false, false, false, false}; // Track which extension keys are pressed
  uint32_t latchToggleModeOnTime = 0;

  void SwapActiveConfig();
  bool ShiftActive();
  void ShiftEventOccured();
  void ShiftClear();
  Color GetOctavePlusColor();
  Color GetOctaveMinusColor();

  bool ChordControlKeyEvent(Point xy, KeypadInfo* keypadInfo);
  bool ArpControlKeyEvent(Point xy, KeypadInfo* keypadInfo);
  bool KeyControlKeyEvent(Point xy, KeypadInfo* keypadInfo);

  void RenderChordControl(Point origin);
  void RenderArpControl(Point origin);
  void RenderKeyControl(Point origin);

public:
  NoteControlBar(Note* notePtr, NotePad* notepad1, NotePad* notepad2, UnderglowLight* underglow1, UnderglowLight* underglow2);

  virtual Dimension GetSize() override {
    return Dimension(8, CTL_BAR_Y);
  }
  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo) override;
  virtual bool Render(Point origin) override;
};