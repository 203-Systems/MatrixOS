#pragma once

#include "MatrixOS.h"
#include "PolyPad.h"
#include "UI/UI.h"
#include "Application.h"

#define POLY_PLAYGROUND_APP_VERSION 1

#define TAG "PolyPlayground"

#define POLY_CONFIGS_HASH StaticHash("203 Systems-PolyPlayground-Configs")

class PolyPlayground : public Application {
public:
  inline static Application_Info info = {
      .name = "Poly Playground",
      .author = "203 Systems",
      .color = Color(0xAEFF00),
      .version = POLY_PLAYGROUND_APP_VERSION,
      .visibility = true,
  };

  CreateSavedVar(TAG, nvsVersion, uint32_t, POLY_PLAYGROUND_APP_VERSION); // In case NoteLayoutConfig got changed

  void Setup(const vector<string>& args) override;

  void KeyEventHandler(InputEvent& inputEvent);

  void GridKeyEvent(Point xy, KeypadInfo* keypadInfo);

  void PolyView();

  void SpacingSelector();
  void ChannelSelector();
  void RootSelector();

  PolyPadConfig polyPadConfig;
};
