#pragma once

#include "MatrixOS.h"
#include "NotePad.h"
#include "Scales.h"
#include "UI/UI.h"
#include "Application.h"
#include "MidiClock.h"

#define NOTE_APP_VERSION 2

#define NOTE_CONFIGS_HASH StaticHash("203 Systems-Note-NotePadConfigs")
#define CUSTOM_SCALES_HASH StaticHash("203 Systems-Note-CustomScales")

enum MidiClockMode : uint8_t {
    CLOCK_INTERNAL,
    CLOCK_INTERNAL_CLOCKOUT,    // Send clock to external devices
    CLOCK_EXTERNAL
};

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
  CreateSavedVar("Note", bpm, uint16_t, 120);
  CreateSavedVar("Note", clockMode, MidiClockMode, CLOCK_INTERNAL);

  MidiClock midiClock = MidiClock(bpm, EFFECT_TPQN);

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

  void Tick();

  void SaveConfigs();

  NotePadConfig notePadConfigs[2];
  NotePadRuntime runtimes[2];

  uint8_t arpMenuPage = 0;  // ARP_BPM

  Color colorPresets[6][2] =
  {
    {0x0040FF, 0x00FFFF},
    {0x8000FF, 0xFF00FF},
    {0xFF4000, 0xFFFF00},
    {0xFF0080, 0xFF60C0},
    {0x00FF00, 0x80FF00},
    {0x40FFFF, 0xFFFFFF}
  };

  static inline uint16_t scales[16] = {MINOR,
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
                         CHROMATIC};

  static inline const string scale_names[16] = {"Minor",
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
                            "Chromatic"};

  static inline uint16_t custom_scales[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
};


