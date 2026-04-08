#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

#include "Sequencer.h"
#include "SequenceEvent.h"
#include "SequenceData.h"

class EventDetailView : public UIComponent {
  Sequencer* sequencer;

  bool wasEnabled = false;

  vector<std::multimap<uint16_t, SequenceEvent>::iterator> eventRefs;
  std::multimap<uint16_t, SequenceEvent>::iterator selectedEventIter;
  SequencePosition position;
  SequencePattern* pattern = nullptr;

  uint32_t lastOnTime = 0;

  // UI parameters for displaying/editing event properties
  uint8_t selectedField = 0; // Which field is being edited
  uint8_t numFields = 0;     // Total editable fields (depends on event type)

public:
  EventDetailView(Sequencer* sequencer);

  virtual bool IsEnabled();
  Dimension GetSize();

  virtual bool KeyEvent(Point xy, KeypadInfo* keypadInfo);

  virtual bool Render(Point origin);

private:
  void RebuildEventList();

  // Event selector (Y=0 row)
  void RenderEventSelector(Point origin);
  bool EventSelectorKeyHandler(Point xy, KeypadInfo* keypadInfo);

  // Micro step selector (Y=1 row)
  void RenderMicroStepSelector(Point origin);
  bool MicroStepSelectorKeyHandler(Point xy, KeypadInfo* keypadInfo);
  bool DeleteEventKeyHandler(Point xy, KeypadInfo* keypadInfo);

  // Note event configuration
  void RenderNoteConfig(Point origin);
  bool NoteConfigKeyHandler(Point xy, KeypadInfo* keypadInfo);

  void RenderLengthSelector(Point origin);
  bool LengthSelectorKeyHandler(Point xy, KeypadInfo* keypadInfo);
  void RenderVelocitySelector(Point origin);
  bool VelocitySelectorKeyHandler(Point xy, KeypadInfo* keypadInfo);

  // CC event configuration
  void RenderCCConfig(Point origin);
  bool CCConfigKeyHandler(Point xy, KeypadInfo* keypadInfo);
};
