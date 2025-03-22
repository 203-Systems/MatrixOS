#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include "applications/BrightnessControl/BrightnessControl.h"
#include "Chord.h"
#include "StrumBar.h"

#define STRUM_APP_VERSION 1

#define STRUM_CONFIGS_HASH StaticHash("203 Systems-Note-NotePadConfigs")

class Strum : public Application {
 public:
  static Application_Info info;

  void Setup() override;
  void Loop() override;
  void KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo);

  CreateSavedVar("Strum", nvsVersion, uint32_t, STRUM_APP_VERSION);  // In case NoteLayoutConfig got changed
  CreateSavedVar("Strum", activeLayout, uint8_t, 0); //0 for left, 1 for right

  void ActionMenu();
  
  StrumBarConfig strumBarConfigs[2] = {
    StrumBarConfig(Color(0x00FFFF), EChord::MAJOR_TRIAD, 0, 3, 0, 200),
    StrumBarConfig(Color(0xFF00FF), EChord::MAJOR_TRIAD, 0, 3, 0, 200)
  };

//   EChord major_chords[7] = {
//     EChord::MAJOR_TRIAD, EChord::MAJOR_SIXTH, EChord::MAJOR_SIXTH_NINTH, EChord::MAJOR_SEVENTH, EChord::MAJOR_NINTH, EChord::MAJOR_ELEVENTH, EChord::MAJOR_THIRTEENTH};
//   string major_chord_names[7] = {"Major", "Major 6th", "Major 6/9", "Major 7th", "Major 9th", "Major 11th", "Major 13th"};

//   EChord minor_chords[7] = {EChord::MINOR_TRIAD, EChord::MINOR_SIXTH, EChord::MINOR_SEVENTH, EChord::MINOR_SEVENTH_FLAT_FIVE, EChord::MINOR_NINTH, EChord::MINOR_ELEVENTH, EChord::MINOR_THIRTEENTH};
//     string minor_chord_names[7] = {"Minor", "Minor 6th", "Minor 7th", "Minor 7th b5", "Minor 9th", "Minor 11th", "Minor 13th"};

//   EChord dominant_chords[4] = {EChord::DOMINANT_SEVENTH, EChord::DOMINANT_NINTH, EChord::DOMINANT_ELEVENTH, EChord::DOMINANT_THIRTEENTH};
//   string dominant_chord_names[4] = {"Dominant 7th", "Dominant 9th", "Dominant 11th", "Dominant 13th"};

//   EChord add_chords[2] = {EChord::ADD_NINE, EChord::ADD_ELEVEN};
//   string add_chord_names[2] = {"Add 9", "Add 11"};

//   EChord suspended_chords[2] = {EChord::SUSPENDED_SECOND, EChord::SUSPENDED_FOURTH};
//   string suspended_chord_names[2] = {"Suspended 2nd", "Suspended 4th"};

//   EChord altered_chords[4] = {EChord::DOMINANT_SEVENTH_SHARP_NINE, EChord::DOMINANT_SEVENTH_FLAT_NINE, EChord::DOMINANT_SEVENTH_SHARP_FIVE, EChord::DOMINANT_SEVENTH_FLAT_FIVE};
//   string altered_chord_names[4] = {"Dominant 7th #9", "Dominant 7th b9", "Dominant 7th #5", "Dominant 7th b5"};

//   EChord diminished_chords[2] = {EChord::DIMINISHED, EChord::DIMINISHED_SEVENTH};
//   string diminished_chord_names[2] = {"Diminished", "Diminished 7th"};

//   EChord augmented_chords[2] = {EChord::AUGMENTED, EChord::AUGMENTED_SEVENTH};
//   string augmented_chord_names[2] = {"Augmented", "Augmented 7th"};
};

inline Application_Info Strum::info = {
    .name = "Strum",
    .author = "203 Systems",
    .color = Color(0xFF80FF),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(Strum);