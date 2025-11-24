#pragma once

#include "MatrixOS.h"
#include "Application.h"
#include <set>

#include "Sequence.h"
#include "SequenceMeta.h"

#define SEQUENCE_VERSION 1
#define SEQUENCER_SLOT_HASH StaticHash("203 Systems-Sequencer-saveSlot")
#define SEQUENCER_DATA_HASH StaticHash("203 Systems-Sequencer-SequenceData")
#define SEQUENCER_META_HASH StaticHash("203 Systems-Sequencer-SequenceMeta")

enum class saveSlotType : uint16_t
{
  OnBoard = 0,
  SDCard = 1,
};

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
  void End();

  // CreateSavedVar("Sequencer", lastSequence, string, "");
  
  SequenceMeta meta;
  Sequence sequence;
  TaskHandle_t tickTaskHandle = nullptr;

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
  void PatternLengthSelector();

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

  // Persistence
  static constexpr uint8_t ONBOARD_SLOT_MAX = 1;
  static constexpr uint8_t SD_SLOT_MAX = 32;
  bool Load(uint16_t slot);
  bool Save(uint16_t slot);
  bool LoadOnBoard(uint16_t slot);
  bool LoadSD(uint16_t slot);
  bool SaveOnBoard(uint16_t slot);
  bool SaveSD(uint16_t slot);
  bool Saved(uint16_t slot);
  bool SavedOnBoard(uint16_t slot);
  bool SavedSD(uint16_t slot);
  SavedVar<uint16_t> saveSlot = SavedVar<uint16_t>(SEQUENCER_SLOT_HASH, 0);

  static void SequenceTask(void* ctx);
};
