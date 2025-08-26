#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
#include "ui/UIUtilities.h"
#include "Application.h"
#include "TemperatureColorPicker.h"


class Lighting : public Application {
 public:
  enum LightingMode {
    RGB,
    Temperature,
    Animation,
    Gradient
  };

  enum ColorEffectMode {
    Static,
    Breath,
    Strobe,
    Saw,
  };

  enum Animations {
    PoliceCar,
    Breathing,
  };

  inline static Application_Info info = {
      .name = "Lighting",
      .author = "203 Systems",
      .color = Color(0xFF00FF),
      .version = 2,
      .visibility = true,
  };
  void Setup() override;
  void Loop() override;
  void KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo);

  void Update();
  Color ApplyColorEffect(Color color, ColorEffectMode effect, uint16_t period, uint16_t start_time);
  Color GetAnimationColor(Animations animation, uint16_t start_time);
  void RenderGradient();
  void Render(Color color);

  void Settings();

  void EffectModeAndSpeedMenu(LightingMode mode);


  CreateSavedVar("Lighting", mode, LightingMode, RGB);
  CreateSavedVar("Lighting", color, Color, Color(0xFFFFFF));
  CreateSavedVar("Lighting", rgb_effect, ColorEffectMode, Static);
  CreateSavedVar("Lighting", rgb_effect_period, uint16_t, 1000);
  CreateSavedVar("Lighting", temperature_color, Color, Color(0xFFFFFF));
  CreateSavedVar("Lighting", temperature_effect, ColorEffectMode, Static);
  CreateSavedVar("Lighting", temperature_effect_period, uint16_t, 1000);
  CreateSavedVar("Lighting", animation, Animations, PoliceCar);
  CreateSavedVar("Lighting", animation_period, uint16_t, 1000);

  Timer renderTimer;
  uint8_t base_layer;
  uint32_t start_time;
};


