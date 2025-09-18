#pragma once

#include "MatrixOS.h"
#include "Application.h"

class MSC : public Application {
  public:
    inline static Application_Info info = {
      .name = "MSC Mode",
      .author = "203 Systems",
      .color = Color(0xFF8000),
      .version = 1,
      .visibility = false,
  };

    void Setup(const vector<string>& args) override;
};