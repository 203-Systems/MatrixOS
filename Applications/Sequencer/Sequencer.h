#pragma once

#include "MatrixOS.h"
#include "Application.h"

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

  CreateSavedVar("Sequencer", lastSequence, string, "");
  
  SequenceMeta meta;
  Sequence sequence;

  uint8_t track = 0;

  void ColorSelector();
  void LayoutSelector();
  void ChannelSelector();
  void BPMSelector();

  void SequencerUI();
  void SequencerMenu();
};