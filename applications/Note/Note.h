#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include "UI/UI.h"
#include "Scales.h"
#include "NotePad.h"

#define APPLICATION_NAME "Note Mode"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0x00FFFF)
#define APPLICATION_VERSION 0
#define APPLICATION_CLASS Note

class Note : public Application {
 public:
  string name = "Note Mode";
  string author = "203 Electronics";
  uint32_t version = 0;

  // Saved Variables
  // CreateSavedVar(APPLICATION_NAME, menuLock, bool, false);

  void Setup() override;
  // void Loop() override;

//   void MidiEventHandler(MidiPacket midiPacket);
//   void NoteHandler(uint8_t channel, uint8_t note, uint8_t velocity);

  void KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo);

  void GridKeyEvent(Point xy, KeyInfo* KeyInfo);
  void IDKeyEvent(uint16_t keyID, KeyInfo* KeyInfo);

  void PlayView();

  void ScaleSelector();

  bool spiltView = false;

  NoteLayoutConfig notePadConfigs[2];
  uint8_t activeConfig = 0;

  uint16_t scales[16] = {
      NATURAL_MINOR,  MAJOR,        DORIAN,     PHRYGIAN,         MIXOLYDIAN,      MELODIC_MINOR_ASCENDING,
      HARMONIC_MINOR, BEBOP_DORIAN, BLUES,      MINOR_PENTATONIC, HUNGARIAN_MINOR, UKRANIAN_DORIAN,
      MARVA,          TODI,         WHOLE_TONE, HIRAJOSHI};

  string scale_names[16] = {
      "Natural Minor",  "Major",        "Dorian",     "Phrygian",         "Mixolydian",      "Melodic Minor Ascending",
      "Harmonic Minor", "Bebop Dorian", "Blues",      "Minor Pentatonic", "Hungarian Minor", "Ukranian Dorian",
      "Marva",          "Todi",         "Whole Tone", "Hirajoshi"};
};

#include "applications/RegisterApplication.h"