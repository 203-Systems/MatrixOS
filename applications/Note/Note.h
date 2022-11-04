#pragma once

#include "MatrixOS.h"
#include "Scales.h"
#include "NotePad.h"
#include "UI/UI.h"
#include "applications/Application.h"

#define APPLICATION_NAME "Note Mode"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0x00FFFF)
#define APPLICATION_VERSION 0
#define APPLICATION_CLASS Note

#define NOTE_CONFIGS_HASH StaticHash("203 Electronics-Note Mode-NotePadConfigs")

class Note : public Application {
 public:
  string name = "Note Mode";
  string author = "203 Electronics";
  uint32_t version = 0;

  // Saved Variables
  CreateSavedVar(APPLICATION_NAME, nvsVersion, uint32_t, version);  // In case NoteLayoutConfig got changed
  CreateSavedVar(APPLICATION_NAME, activeConfig, uint8_t, 0);
  CreateSavedVar(APPLICATION_NAME, spiltView, bool, false);

  void Setup() override;

  void KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo);

  void GridKeyEvent(Point xy, KeyInfo* KeyInfo);
  void IDKeyEvent(uint16_t keyID, KeyInfo* KeyInfo);

  void PlayView();

  void ScaleSelector();
  void OverlapSelector();
  void ChannelSelector();
  void ColorSelector();

  NoteLayoutConfig notePadConfigs[2];

  uint16_t scales[16] = {NATURAL_MINOR,   MAJOR, DORIAN, PHRYGIAN,   MIXOLYDIAN, MELODIC_MINOR_ASCENDING, HARMONIC_MINOR, BEBOP_DORIAN, BLUES, MINOR_PENTATONIC, HUNGARIAN_MINOR,
                         UKRANIAN_DORIAN, MARVA, TODI,   WHOLE_TONE, HIRAJOSHI};

  string scale_names[16] = {"Natural Minor",  "Major",        "Dorian",     "Phrygian",         "Mixolydian",      "Melodic Minor Ascending",
                            "Harmonic Minor", "Bebop Dorian", "Blues",      "Minor Pentatonic", "Hungarian Minor", "Ukranian Dorian",
                            "Marva",          "Todi",         "Whole Tone", "Hirajoshi"};
};

#include "applications/RegisterApplication.h"