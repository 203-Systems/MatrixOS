#pragma once

#include "MatrixOS.h"
#include "Application.h"
#include <set>

#include "Sequence.h"
#include "SequenceMeta.h"

#define SEQUENCE_VERSION 1


class Sequencer : public Application {
 public:
  inline static Application_Info info = {
      .name = "Sequencer",
      .author = "203 Systems",
      .color = Color::White,
      .version = 1,
      .visibility = true,
  };

  void Setup(const vector<string>& args) override;

  // CreateSavedVar("Sequencer", lastSequence, string, "");
  
  SequenceMeta meta;
  Sequence sequence;

  uint8_t track = 0;

  // UI state
  bool patternView = false;
  bool clear = false;
  bool copy = false;
  bool shift[2] = {false, false};
  uint32_t shiftOnTime = 0;
  bool shiftEventOccured[2] = {false, false};

  // Helper Var
  bool patternViewActive = false; // True if the patternView is on, even if unlatched, shift hold

  enum class ViewMode
  {
    Sequencer,
    PatternSetting,
    StepDetail,
    Mixer,
    Session
  };

  ViewMode currentView = ViewMode::Sequencer;

  std::set<uint8_t> stepSelected;

  std::unordered_map<uint8_t, uint8_t> noteSelected;
  std::unordered_multiset<uint8_t> noteActive;

  void ColorSelector();
  void LayoutSelector();
  void ChannelSelector();
  void BPMSelector();
  void SwingSelector();
  void BarLengthSelector();

  void SequencerUI();
  void SequencerMenu();

  bool ClearActive();
  bool CopyActive();
  bool ShiftActive();
  void ShiftEventOccured();

  void ClearActiveNotes();
  void ClearSelectedNotes();

  bool IsNoteActive(uint8_t note) const;

  void SetView(ViewMode view);
};
