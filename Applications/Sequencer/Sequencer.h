#pragma once

#include "MatrixOS.h"
#include "Application.h"

#include "SequenceData.h"

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
  void Loop() override;

  SequenceMeta meta;
  SequenceData data;

  uint8_t track = 0;
};