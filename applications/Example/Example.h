/*
This is the example application for Matrix OS
Remember to include this header file in the UserApplications.h in the Applications folder
*/

#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"

class ExampleAPP : public Application {
 public:
  static Application_Info info;

  void Setup() override;
  void Loop() override;
};

inline Application_Info ExampleAPP::info = {
    .name = "Example",
    .author = "203 Electronics",
    .color = Color(0xFFFFFF),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(ExampleAPP);