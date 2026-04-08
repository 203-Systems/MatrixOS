#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include <functional>

#include "Sequencer.h"

class SequencerNotePad; // Forward declaration

class SequencerControlBar : public UIComponent {
  Sequencer* sequencer;
  SequencerNotePad* notePad;

  const uint8_t OctaveGradient[8] = {0, 16, 42, 68, 124, 182, 255};

  bool HandleShiftKey(uint8_t idx, bool right, KeypadInfo* keypadInfo);
  bool HandlePlayKey(KeypadInfo* keypadInfo);
  bool HandleTrackPlayKey(KeypadInfo* keypadInfo);
  bool HandleStepPlayKey(KeypadInfo* keypadInfo);
  bool HandleResumeKey(KeypadInfo* keypadInfo);
  bool HandleRecordKey(KeypadInfo* keypadInfo);
  bool HandleSessionKey(KeypadInfo* keypadInfo);
  bool HandleMixerKey(KeypadInfo* keypadInfo);
  bool HandleClearKey(KeypadInfo* keypadInfo);
  bool HandleCopyKey(KeypadInfo* keypadInfo);
  bool HandleNudgeKey(bool positive, KeypadInfo* keypadInfo);
  bool HandleOctaveOffsetKey(bool positive, KeypadInfo* keypadInfo);
  bool HandleStepOctaveOffsetKey(bool positive, KeypadInfo* keypadInfo);
  bool HandleQuantizeKey(KeypadInfo* keypadInfo);
  bool HandleTwoPatternToggleKey(KeypadInfo* keypadInfo);

public:
  SequencerControlBar(Sequencer* sequencer, SequencerNotePad* notePad);

  Dimension GetSize();
  Color GetOctavePlusColor();
  Color GetOctaveMinusColor();
  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo);
  virtual bool Render(Point origin);
};
