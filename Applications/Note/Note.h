#pragma once

#include "MatrixOS.h"
#include "NotePad.h"
#include "Scales.h"
#include "UI/UI.h"
#include "Application.h"

#define NOTE_APP_VERSION 2

#define NOTE_CONFIGS_HASH StaticHash("203 Systems-Note-NotePadConfigs")

class Note : public Application {
 public:
  inline static Application_Info info = {
      .name = "Note",
      .author = "203 Systems",
      .color =  Color(0x00FFFF),
      .version = NOTE_APP_VERSION,
      .visibility = true,
  };
  
  enum ESpiltView : uint8_t { SINGLE_VIEW, VERT_SPLIT, HORIZ_SPLIT};
  // Saved Variables
  CreateSavedVar("Note", nvsVersion, uint32_t, NOTE_APP_VERSION);  // In case NotePadConfig got changed
  CreateSavedVar("Note", activeConfig, uint8_t, 0);
  CreateSavedVar("Note", splitView, ESpiltView, SINGLE_VIEW);
  CreateSavedVar("Note", controlBar, bool, false);

  void Setup(const vector<string>& args) override;

  void KeyEventHandler(KeyEvent& keyEvent);

  void GridKeyEvent(Point xy, KeyInfo* KeyInfo);
  void IDKeyEvent(uint16_t keyID, KeyInfo* KeyInfo);

  void PlayView();

  void ScaleSelector();
  void LayoutSelector();
  void ChannelSelector();
  void ColorSelector();
  void ArpConfigMenu();

  NotePadConfig notePadConfigs[2];
  NotePad *activeNotePads[2] = {nullptr, nullptr};

  Color colorPresets[6][2] =
  {
    {0x0040FF, 0x00FFFF},
    {0x8000FF, 0xFF00FF},
    {0xFF4000, 0xFFFF00},
    {0xFF0080, 0xFF60C0},
    {0x00FF00, 0x80FF00},
    {0x40FFFF, 0xFFFFFF}
  };

  static inline uint16_t scales[32] = {NATURAL_MINOR,
                         MAJOR,
                         DORIAN,
                         PHRYGIAN,
                         MIXOLYDIAN,
                         MELODIC_MINOR_ASCENDING,
                         HARMONIC_MINOR,
                         BEBOP_DORIAN,
                         BLUES,
                         MINOR_PENTATONIC,
                         HUNGARIAN_MINOR,
                         UKRAINIAN_DORIAN,
                         MARVA,
                         TODI,
                         WHOLE_TONE,
                         CHROMATIC,
                         LYDIAN,
                         LOCRIAN,
                         MAJOR_PENTATONIC,
                         PHRYGIAN_DOMINANT,
                         HALF_WHOLE_DIMINISHED,
                         MIXOLYDIAN_BEBOP,
                         SUPER_LOCRIAN,
                         HIRAJOSHI,
                         IN_SEN,
                         YO_SCALE,
                         IWATO,
                         WHOLE_HALF,
                         BEBOP_MINOR,
                         MAJOR_BLUES,
                         KUMOI,
                         BEBOP_MAJOR};

  static inline const string scale_names[32] = {"Natural Minor",
                            "Major",
                            "Dorian",
                            "Phrygian",
                            "Mixolydian",
                            "Melodic Minor Ascending",
                            "Harmonic Minor",
                            "Bebop Dorian",
                            "Blues",
                            "Minor Pentatonic",
                            "Hungarian Minor",
                            "Ukrainian Dorian",
                            "Marva",
                            "Todi",
                            "Whole Tone",
                            "Chromatic",
                            "Lydian",
                            "Locrian",
                            "Major Pentatonic",
                            "Phrygian Dominant",
                            "Half-Whole Diminished",
                            "Mixolydian BeBop",
                            "Super Locrian",
                            "Hirajoshi",
                            "In Sen",
                            "Yo Scale",
                            "Iwato",
                            "Whole Half",
                            "BeBop Minor",
                            "Major Blues",
                            "Kumoi",
                            "BeBop Major"};
};


