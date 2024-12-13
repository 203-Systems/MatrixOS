#pragma once

#include "MatrixOS.h"
#include "PolyPad.h"
#include "ui/UI.h"
#include "applications/Application.h"

#define POLY_PLAYGROUND_APP_VERSION 1

#define TAG "PolyPlayground"

#define POLY_CONFIGS_HASH StaticHash("203 Systems-PolyPlayground-Configs")

class PolyPlayground : public Application {
 public:
  static Application_Info info;

  CreateSavedVar(TAG, nvsVersion, uint32_t, POLY_PLAYGROUND_APP_VERSION);  // In case NoteLayoutConfig got changed

  void Setup() override;

  void KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo);

  void GridKeyEvent(Point xy, KeyInfo* KeyInfo);
  void IDKeyEvent(uint16_t keyID, KeyInfo* KeyInfo);

  void PolyView();

  void SpacingSelector();
  void ChannelSelector();
  void RootSelector();

  PolyPadConfig polyPadConfig;
};

inline Application_Info PolyPlayground::info = {
    .name = "Poly Playground",
    .author = "203 Systems",
    .color =  Color(0xAEFF00),
    .version = POLY_PLAYGROUND_APP_VERSION,
    .visibility = true,
};

REGISTER_APPLICATION(PolyPlayground);
