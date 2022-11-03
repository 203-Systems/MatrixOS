#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include "UI/UI.h"

#define APPLICATION_NAME "Note Mode"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0x00FFFF)
#define APPLICATION_VERSION 0
#define APPLICATION_CLASS Note

//Bit map for scales, LSB(Bit0) is C and Bit11 is B. First 4 bits are reserved
enum EScale : uint16_t {
  NATURAL_MINOR = 0b010110101101,
  MAJOR = 0b101010110101,
  DORIAN = 0b011010101101,
  PHRYGIAN = 0b010110101011,
  MIXOLYDIAN = 0b011010110101,
  MELODIC_MINOR_ASCENDING = 0b101010101101,
  HARMONIC_MINOR = 0b100110101101,
  BEBOP_DORIAN = 0b011010111001,
  BLUES = 0b010011101001,
  MINOR_PENTATONIC = 0b010010101001,
  HUNGARIAN_MINOR = 0b100111001101,
  UKRANIAN_DORIAN = 0b011011001101,
  MARVA = 0b101011010011,
  TODI = 0b100111001011,
  WHOLE_TONE = 0b010101010101,
  HIRAJOSHI = 0b000110001101,
  CHROMATIC = 0b111111111111,
};

struct NoteLayoutConfig
{
  uint8_t rootKey = 0;
  uint16_t scale = NATURAL_MINOR;
  int8_t octane = 0;
  uint8_t channel = 0;
  Color color = Color(0x0040FF);
  Color rootColor = Color(0x00FFFF);
};

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

  void ActionMenu();

  NoteLayoutConfig configs[2];
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