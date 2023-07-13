#pragma once

#include "MatrixOS.h"
#include "NotePad.h"
#include "Scales.h"
#include "ui/UI.h"
#include "applications/Application.h"

#define NOTE_APP_VERSION 1

#define NOTE_CONFIGS_HASH StaticHash("203 Electronics-Note-NotePadConfigs")

class Note : public Application {
 public:
  static Application_Info info;

  // Saved Variables
  CreateSavedVar("Note", nvsVersion, uint32_t, NOTE_APP_VERSION);  // In case NoteLayoutConfig got changed
  CreateSavedVar("Note", activeConfig, uint8_t, 0);
  CreateSavedVar("Note", splitView, bool, false);

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

  uint16_t scales[32] = {NATURAL_MINOR,
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
                         UKRANIAN_DORIAN,
                         MARVA,
                         TODI,
                         WHOLE_TONE,
                         CHROMATIC,
                         LYDIAN,
                         LOCRIAN,
                         MAJOR_PENTATONIC,
                         PHYRIGIAN_DOMINATE,
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

  string scale_names[32] = {"Natural Minor",
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
                            "Ukranian Dorian",
                            "Marva",
                            "Todi",
                            "Whole Tone",
                            "Chromatic",
                            "Lydian",
                            "Locrian",
                            "Major Pentatonic",
                            "Phyrigian Dominate",
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

inline Application_Info Note::info = {
    .name = "Note",
    .author = "203 Electronics",
    .color =  Color(0x00FFFF),
    .version = NOTE_APP_VERSION,
    .visibility = true,
};

REGISTER_APPLICATION(Note);
