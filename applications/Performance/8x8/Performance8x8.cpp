#include "Performance8x8.h"

void Performance::Setup() {
  // Load variable
  //  MatrixOS
  canvasLedLayer = MatrixOS::LED::CurrentLayer();
  currentKeymap = compatibilityMode;
}

void Performance::Loop() {
  if (stfuTimer.Tick(10))
  { stfuScan(); }

  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEvent(keyEvent.id, &keyEvent.info); }

  struct MidiPacket midiPacket;
  while (MatrixOS::MIDI::Get(&midiPacket))
  { MidiEvent(midiPacket); }
}

void Performance::MidiEvent(MidiPacket midiPacket) {
  // MatrixOS::Logging::LogDebug("Performance", "Midi Recived %d %d %d %d", midiPacket.status, midiPacket.data[0],
  // midiPacket.data[1], midiPacket.data[2]);
  switch (midiPacket.status)
  {
    case NoteOn:
    case ControlChange:
      NoteHandler(midiPacket.channel(), midiPacket.note(), midiPacket.velocity());
      break;
    case NoteOff:
      NoteHandler(midiPacket.channel(), midiPacket.note(), 0);
      break;
    default:
      break;
  }
}

Point Performance::NoteToXY(uint8_t note) {
  switch (currentKeymap)
  {
    case 0:
    {
      if (note > 35 && note < 100)
      {
        uint8_t xy_raw = user1_keymap_optimized[note - 36];
        return Point(xy_raw >> 4, xy_raw & 0x0f);
      }
      else if (note > 99 && note < 108)  // Side Light Right Column
      { return Point(8, note - 100); }
      else if (note > 115 && note < 124)  // Side Light Bottom Row
      { return Point(note - 116, 8); }
      else if (note > 107 && note < 116)  // Side Light Left Column
      { return Point(-1, note - 108); }
      else if (note > 27 && note < 36)  // Side Light Top Row
      { return Point(note - 28, -1); }
      break;
    }
    case 1:
    {
      int8_t x = (note % 10) - 1;
      int8_t y = 8 - ((note / 10));
      return Point(x, y);
    }
  }
  return Point::Invalid();
}

// Returns -1 when no note
int8_t Performance::XYToNote(Point xy) {
  switch (currentKeymap)
  {
    case 0:
    {
      if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)
      { return keymap[currentKeymap][xy.y][xy.x]; }
      else if (xy.y == -1 && xy.x >= 0 && xy.x < 8)  // TouchBar Top Row
      { return touch_keymap[currentKeymap][0][xy.x]; }
      else if (xy.x == 8 && xy.y >= 0 && xy.y < 8)  // TouchBar Right Column
      { return touch_keymap[currentKeymap][1][xy.y]; }
      else if (xy.y == 8 && xy.x >= 0 && xy.x < 8)  // TouchBar Bottom Row
      { return touch_keymap[currentKeymap][2][xy.x]; }
      else if (xy.x == -1 && xy.y >= 0 && xy.y < 8)  // TouchBar Left Column
      { return touch_keymap[currentKeymap][3][xy.y]; }
      else
      {
        return -1;  // No suitable keymap
      }
    }
    case 1:
    {
      return (8 - xy.y) * 10 + (xy.x + 1);
    }
  }
  return -1;
}

void Performance::NoteHandler(uint8_t channel, uint8_t note, uint8_t velocity) {
  // MatrixOS::Logging::LogDebug("Performance", "Midi Recivied %#02X %#02X %#02X", channel, note, velocity);
  Point xy = NoteToXY(note);

  if (compatibilityMode)
  {
    channel = 1;  // So it will use legacy palette
  }

  if (xy && !(velocity == 0 && stfu))
  {
    // MatrixOS::Logging::LogDebug("Performance", "Set LED");
    MatrixOS::LED::SetColor(xy, palette[channel % 2][velocity], canvasLedLayer);
    MatrixOS::LED::Update(canvasLedLayer);
  }
  // else if(!xy)
  // {
  //     MatrixOS::Logging::LogDebug("Performance", "XY incorrect");
  // }
  // else if((velocity == 0 && stfu))
  // {
  //     MatrixOS::Logging::LogDebug("Performance", "STFU");
  // }
  if (stfu)
  {
    if (velocity == 0)
    { stfuMap[note] = stfu; }
    else
    { stfuMap[note] = -1; }
  }
}

void Performance::KeyEvent(uint16_t KeyID, KeyInfo* keyInfo) {
  Point xy = MatrixOS::KEYPAD::ID2XY(KeyID);
  if (xy)  // IF XY is vaild, means it's on the main grid
  { GridKeyEvent(xy, keyInfo); }
  else  // XY Not vaild,
  { IDKeyEvent(KeyID, keyInfo); }
}

void Performance::GridKeyEvent(Point xy, KeyInfo* keyInfo) {
  // MatrixOS::Logging::LogDebug("Performance Mode", "KeyEvent %d %d", xy.x, xy.y);
  int8_t note = XYToNote(xy);

  if (note == -1)
  { return; }

  if (keyInfo->state == PRESSED)
  { MatrixOS::MIDI::Send(MidiPacket(0, NoteOn, 0, note, keyInfo->velocity.to7bits())); }
  else if (keyInfo->state == AFTERTOUCH)
  { MatrixOS::MIDI::Send(MidiPacket(0, AfterTouch, 0, note, keyInfo->velocity.to7bits())); }
  else if (keyInfo->state == RELEASED)
  { MatrixOS::MIDI::Send(MidiPacket(0, compatibilityMode ? NoteOn : NoteOff, 0, note, keyInfo->velocity.to7bits())); }
}

void Performance::IDKeyEvent(uint16_t keyID, KeyInfo* keyInfo) {
  // MatrixOS::Logging::LogDebug(name, "Key Event");
  if (keyID == 0 && keyInfo->state == (menuLock ? HOLD : PRESSED))
  { ActionMenu(); }
}

void Performance::stfuScan() {
  for (uint8_t note = 0; note < 128; note++)
  {
    if (stfuMap[note] > 0)
    { stfuMap[note]--; }
    else if (stfuMap[note] == 0)
    {
      Point xy = NoteToXY(note);
      if (xy)
      {
        MatrixOS::LED::SetColor(xy, 0, canvasLedLayer);
        MatrixOS::LED::Update(canvasLedLayer);
      }
      stfuMap[note] = -1;
    }
  }
}

void Performance::ActionMenu() {
  MatrixOS::Logging::LogDebug(name, "Enter Action Menu");

  UI actionMenu("Action Menu", Color(0x00FFAA), true);

  UIButtonLarge brightnessBtn(
      "Brightness", Color(0xFFFFFF), Dimension(2, 2), [&]() -> void { MatrixOS::SYS::NextBrightness(); },
      [&]() -> void { BrightnessControl().Start(); });
  actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButtonLarge clearCanvasBtn("Clear Canvas", Color(0x00FF00), Dimension(2, 1),
                               [&]() -> void { MatrixOS::LED::Fill(0, canvasLedLayer); });
  actionMenu.AddUIComponent(clearCanvasBtn, Point(3, 2));

  UIButtonLarge rotatRightBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2),
                              [&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  actionMenu.AddUIComponent(rotatRightBtn, Point(5, 3));

  UIButtonLarge rotateDownBtn("Rotate to this side", Color(0x00FF00), Dimension(2, 1),
                              [&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  actionMenu.AddUIComponent(rotateDownBtn, Point(3, 5));

  UIButtonLarge rotateLeftBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2),
                              [&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  actionMenu.AddUIComponent(rotateLeftBtn, Point(2, 3));

  // Note Pad
  UINotePad notePad(Dimension(8, 2), keymap_color[currentKeymap], keymap_channel[currentKeymap],
                    (uint8_t*)note_pad_map[currentKeymap]);
  actionMenu.AddUIComponent(notePad, Point(0, 6));

  // Other Controls
  UIButton systemSettingBtn("System Setting", Color(0xFFFFFF), [&]() -> void { MatrixOS::SYS::OpenSetting(); });
  actionMenu.AddUIComponent(systemSettingBtn, Point(7, 5));

  UIButtonDimmable menuLockBtn(
      "Menu Lock", Color(0xA0FF00), [&]() -> bool { return menuLock; }, [&]() -> void { menuLock = !menuLock; });
  actionMenu.AddUIComponent(menuLockBtn, Point(0, 5));  // Current the currentKeymap is directly linked to
                                                        // compatibilityMode. Do we really need > 2 keymap tho?

  UIButtonDimmable flickerReductionBtn(
      "Flicker Reduction", Color(0xAAFF00), [&]() -> bool { return stfu; },
      [&]() -> void { stfu = bool(!stfu) * STFU_DEFAULT; });
  actionMenu.AddUIComponent(flickerReductionBtn, Point(0, 0));  // Current the currentKeymap is directly linked to
                                                                // compatibilityMode. Do we really need > 2 keymap tho?

  UIButtonDimmable compatibilityModeBtn(
      "Compatibility Mode", Color(0xFFFF00), [&]() -> bool { return compatibilityMode; },
      [&]() -> void {
        compatibilityMode = !compatibilityMode;
        currentKeymap = compatibilityMode;
        notePad.SetColor(keymap_color[currentKeymap]);
        notePad.SetChannel(keymap_channel[currentKeymap]);
        notePad.SetMap((uint8_t*)note_pad_map[currentKeymap]);
      });
  actionMenu.AddUIComponent(compatibilityModeBtn, Point(7, 0));  // Current the currentKeymap is directly linked to
                                                                 // compatibilityMode. Do we really need > 2 keymap
                                                                 // tho?

  actionMenu.AddFuncKeyHold([&]() -> void { Exit(); });
  actionMenu.SetLoopFunc([&]() -> void { 
      struct MidiPacket midiPacket;
      while (MatrixOS::MIDI::Get(&midiPacket))
      { MidiEvent(midiPacket); }
  });

  actionMenu.Start();

  MatrixOS::Logging::LogDebug(name, "Exit Action Menu");
}

// #endif
