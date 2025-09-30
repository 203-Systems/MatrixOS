#pragma once

#include "MatrixOS.h"
#include "Scales.h"
#include "UI/UI.h"
#include "MidiPipeline.h"
#include "NoteLatch.h"
#include "ChordEffect.h"
#include "Arpeggiator.h"

enum NoteLayoutMode : uint8_t {
  OCTAVE_LAYOUT,
  OFFSET_LAYOUT,
  CHROMATIC_LAYOUT,
  PIANO_LAYOUT,
};

enum NoteType : uint8_t {
  ROOT_NOTE,
  SCALE_NOTE,
  OFF_SCALE_NOTE,
};

enum ColorMode : uint8_t {
  ROOT_N_SCALE,
  COLOR_PER_KEY_POLY,
  COLOR_PER_KEY_RAINBOW,
};

extern const Color polyNoteColor[12];
extern const Color rainbowNoteColor[12];

struct ActiveKey {
  Point position;
  Fract16 velocity;

  ActiveKey(Point pos, Fract16 vel) : position(pos), velocity(vel) {}
};

struct NotePadConfig {
  uint8_t rootKey = 0;
  uint16_t scale = NATURAL_MINOR;
  int8_t octave = 4;
  uint8_t channel = 0;
  NoteLayoutMode mode = OCTAVE_LAYOUT;
  bool enforceScale = true;
  union {
    struct // Octave Mode
    {
      uint8_t unknown = 0;
    };
    struct // Offset Mode
    {
      uint8_t x_offset : 4;     // X offset for the note pad
      uint8_t y_offset : 4;     // Y offset for the note pad
    };
    struct // Piano Mode
    {
      uint8_t unknown2;
    };
  };
  bool forceSensitive = true;
  Color rootColor = Color(0x0040FF);
  Color color = Color(0x00FFFF);
  ColorMode colorMode = ROOT_N_SCALE;
  bool useWhiteAsOutOfScale = false;
  ArpeggiatorConfig arpConfig;
};

struct NotePadRuntime
{
  NotePadConfig* config = nullptr;
  NoteLatch noteLatch;
  ChordEffect chordEffect;
  ArpeggiatorConfig* arpConfig = nullptr;
  Arpeggiator arpeggiator;
  MidiPipeline midiPipeline;
  uint8_t activeNotes[64] = {0}; // Each uint8_t stores two 4-bit counters (upper/lower nibble)

  NotePadRuntime() : arpeggiator(nullptr) {}
};

class NotePad : public UIComponent {
 public:
  Dimension dimension;
  std::vector<uint8_t> noteMap;
  uint16_t c_aligned_scale_map;
  NotePadRuntime* rt;
  bool first_scan = true;
  std::vector<ActiveKey> activeKeys;

  NotePad(Dimension dimension, NotePadRuntime* data);
  ~NotePad();

  void Tick();

  virtual Color GetColor();
  virtual Dimension GetSize();

  NoteType InScale(int16_t note);
  int16_t NoteFromRoot(int16_t note);
  int16_t GetNextInScaleNote(int16_t note);

  bool IsNoteActive(uint8_t note);
  uint8_t GetActiveNoteCount(uint8_t note);
  void SetActiveNoteCount(uint8_t note, uint8_t count);
  void IncrementActiveNote(uint8_t note);
  void DecrementActiveNote(uint8_t note);

  void AddActiveKey(Point position, Fract16 velocity);
  void RemoveActiveKey(Point position);
  void UpdateActiveKeyVelocity(Point position, Fract16 velocity);

  void GenerateOctaveKeymap();
  void GenerateOffsetKeymap();
  void GenerateChromaticKeymap();
  void GeneratePianoKeymap();
  void GenerateKeymap();

  bool RenderRootNScale(Point origin);
  bool RenderColorPerKey(Point origin);

  void FirstScan(Point origin);
  
  virtual bool Render(Point origin) override;
  virtual bool KeyEvent(Point xy, KeyInfo* keyInfo) override;

  void SetDimension(Dimension dimension);
  void SetPadRuntime(NotePadRuntime* rt);
};