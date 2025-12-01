#pragma once

#include "MatrixOS.h"
#include "Application.h"
#include <set>

#include "Sequence.h"
#include "SequenceMeta.h"

enum class SequencerMessage
{
    NONE,
    CLEAR,
    CLEARED,
    COPY,
    COPIED,
    NUDGE,
    QUANTIZE,
    QUANTIZED,
    TWO_PATTERN_VIEW,
    CLIP,
    MIX,
    PLAY,
    RECORD,
    UNDO,
    UNDONE,
};

enum class SequenceSelectionType
{
    NONE,
    STEP,
    PATTERN,
    CLIP
};

struct SequenceCopySource
{
    SequenceSelectionType type = SequenceSelectionType::NONE;
    int8_t track = -1;
    int8_t clip = -1;
    int8_t pattern = -1;
    int8_t step = -1;

    void Clear()
    {
        type = SequenceSelectionType::NONE;
        track = -1;
        clip = -1;
        pattern = -1;
        step = -1;
    }

    bool Selected() const
    {
        return type != SequenceSelectionType::NONE;
    }

    bool IsType(SequenceSelectionType selectionType) const
    {
        return type == selectionType;
    }
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
  bool wideClipMode = false;
  uint8_t clipWindow = 0;

  // Helper Var
  bool patternViewActive = false; // True if the patternView is on, even if unlatched, shift hold

  // EventDisplay
  SequencerMessage lastMessage = SequencerMessage::NONE;
  uint32_t lastMessageTime = 0;

  enum class ViewMode
  {
    Sequencer,
    PatternSetting,
    StepDetail,
    Mixer,
    Session
  };

  ViewMode currentView = ViewMode::Sequencer;

  std::set<std::pair<uint8_t, uint8_t>> stepSelected; // pair<pattern, step>
  SequenceCopySource copySource;

  std::unordered_map<uint8_t, uint8_t> noteSelected;
  std::unordered_multiset<uint8_t> noteActive;
  bool activeTrackSelected = false;

  void ColorSelector();
  void LayoutSelector();
  void ChannelSelector();
  void BPMSelector();
  void SwingSelector();
  void TimeSignatureSelector();
  void PatternLengthSelector();

  void SequencerUI();
  void SequencerMenu();
  void ClearState();

  bool ClearActive();
  bool CopyActive();
  bool ShiftActive();
  void ShiftEventOccured();

  void ClearActiveNotes();
  void ClearSelectedNotes();

  bool IsNoteActive(uint8_t note) const;

  void SetView(ViewMode view);
  void SetMessage(SequencerMessage msg, bool stayOn = false);

  // Persistence
  static constexpr uint8_t SD_SLOT_MAX = 48;
  bool Load(uint16_t slot);
  bool Save(uint16_t slot);
  bool Saved(uint16_t slot);
  CreateSavedVar("Sequencer", saveSlot, uint16_t, 0xFFFF);

  void ConfirmSaveUI();
  void SequenceBrowser();
  bool ClearSlot(uint16_t slot);
  bool CopySlot(uint16_t from, uint16_t to);
  bool BackupSlot(uint16_t slot);
  
  static void SequenceTask(void* ctx);
};
