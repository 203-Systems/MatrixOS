/*
This is the example application for Matrix OS
Remember to include this header file in the Applications.h in your device family folder (devices/<Device Family>/Applications.h)

What this application does:
Any pressed will lit up the key (in a user defined color)
Click the function key will open an menu UI
Top left button will open a number selector UI that saves teh value into the number variable
Top right button will open a color picker UI that saves the value into the color variable. This also changes the color of the button pressed
Click the function key in the menu will exit the UI
Hold the function key in the menu will exit the application
Midi signal recived will be echoed back to the host
*/

#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"

class Dice : public Application {
 public:
  static Application_Info info;

  enum UnderglowEffectMode {
    Static,
    Off,
    Breath,
    Strobe,
    Saw,
  };

  uint32_t rolling_start_time = 0;
  uint32_t last_roll_time = 0;

  enum DicePhase {
    Rolling,
    Confirmed,
  };

  enum DiceMode {
    Dot,
    Number
  };

  void Setup() override;
  void Loop() override;


  CreateSavedVar("Dice", mode, DiceMode, Dot);

  // Dot Mode
  CreateSavedVar("Dice", rolling_color, Color, Color(0xFFFFFF));
  CreateSavedVar("Dice", confirmed_color, Color, Color(0xFFFFFF));
  CreateSavedVar("Dice", dot_faces, uint8_t, 6);
  CreateSavedVar("Dice", rolling_speed, uint8_t, 30);
  CreateSavedVar("Dice", flashing_speed, uint8_t, 10);
  CreateSavedVar("Dice", rolling_rainbow_mode, bool, true);
  CreateSavedVar("Dice", confirmed_rainbow_mode, bool, false);
  CreateSavedVar("Dice", rolling_underglow_mode, UnderglowEffectMode, Saw);
  CreateSavedVar("Dice", confirmed_underglow_mode, UnderglowEffectMode, Breath);
  CreateSavedVar("Dice", rolling_underglow_effect_period, uint16_t, 300);
  CreateSavedVar("Dice", confirmed_underglow_effect_period, uint16_t, 1000);

  // Number Mode
  CreateSavedVar("Dice", number_faces, uint8_t, 30);


  void Settings();
  void KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo);

  uint8_t GetRandomNumber(uint8_t upperbound, uint8_t lowerbound = 1);
  void RenderDot(Point point, Color color);
  void RenderDots(uint8_t number, Color color);
  void RenderNumber(Point point, uint8_t number, Color color);
  void RenderNumbers(uint8_t number, Color color);
  void RollDice();
  void RenderUnderglow(UnderglowEffectMode mode, Color color, uint8_t period);
  void FaceSelector();

  void DotFaceSelector();
  
  Color ApplyColorEffect(Color color, UnderglowEffectMode effect, uint16_t period, uint16_t start_time);
  void UnderglowEffectModeAndSpeedMenu(DicePhase phase);

  Timer renderTimer;
  uint32_t timestamp = 0;
  uint8_t rolled_number = 1;
  DicePhase current_phase = Rolling;

  const uint8_t number_font[40] = {
      // #48 Number '0'.
      0x1E,  // ░░░▓▓▓▓░
      0x29,  // ░░▓░▓░░▓
      0x25,  // ░░▓░░▓░▓
      0x1E,  // ░░░▓▓▓▓░

      // #49 Number '1'.
      0x22,  // ░░▓░░░▓░
      0x3F,  // ░░▓▓▓▓▓▓
      0x20,  // ░░▓░░░░░
      0x00,  // ░░░░░░░░

      // #50 Number '2'.
      0x32,  // ░░▓▓░░▓░
      0x29,  // ░░▓░▓░░▓
      0x29,  // ░░▓░▓░░▓
      0x26,  // ░░▓░░▓▓░

      // #51 Number '3'.
      0x12,  // ░░░▓░░▓░
      0x21,  // ░░▓░░░░▓
      0x25,  // ░░▓░░▓░▓
      0x1A,  // ░░░▓▓░▓░

      // #52 Number '4'.
      0x0C,  // ░░░░▓▓░░
      0x0A,  // ░░░░▓░▓░
      0x3F,  // ░░▓▓▓▓▓▓
      0x08,  // ░░░░▓░░░

      // #53 Number '5'.
      0x17,  // ░░░▓░▓▓▓
      0x25,  // ░░▓░░▓░▓
      0x25,  // ░░▓░░▓░▓
      0x19,  // ░░░▓▓░░▓

      // #54 Number '6'.
      0x1E,  // ░░░▓▓▓▓░
      0x25,  // ░░▓░░▓░▓
      0x25,  // ░░▓░░▓░▓
      0x18,  // ░░░▓▓░░░

      // #55 Number '7'.
      0x01,  // ░░░░░░░▓
      0x39,  // ░░▓▓▓░░▓
      0x05,  // ░░░░░▓░▓
      0x03,  // ░░░░░░▓▓

      // #56 Number '8'.
      0x1A,  // ░░░▓▓░▓░
      0x25,  // ░░▓░░▓░▓
      0x25,  // ░░▓░░▓░▓
      0x1A,  // ░░░▓▓░▓░

      // #57 Number '9'.
      0x06,  // ░░░░░▓▓░
      0x29,  // ░░▓░▓░░▓
      0x29,  // ░░▓░▓░░▓
      0x1E,  // ░░░▓▓▓▓░

  };
};

// Meta data about this application
inline Application_Info Dice::info = {
    .name = "Dice",
    .author = "203 Electronics",
    .color = Color(0xFFA500),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(Dice);