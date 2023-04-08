#include "Performance8x8.h"

void Performance::Setup() {
  // Load variable
  canvasLedLayer = MatrixOS::LED::CurrentLayer();
  currentKeymap = compatibilityMode;
}

void Performance::Loop() {
  if (stfu && stfuTimer.Tick(10))
  { stfuScan(); }

  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }

  struct MidiPacket midiPacket;
  while (MatrixOS::MIDI::Get(&midiPacket))
  { MidiEventHandler(midiPacket); }
}

void Performance::MidiEventHandler(MidiPacket midiPacket) {
  // MLOGD("Performance", "Midi Recived %d %d %d %d", midiPacket.status, midiPacket.data[0],
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
    case SysExData:
    case SysExEnd:
      SysExHandler(midiPacket);
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
  // MLOGD("Performance", "Midi Recivied %#02X %#02X %#02X", channel, note, velocity);
  Point xy = NoteToXY(note);

  if (compatibilityMode)
  {
    channel = 1;  // So it will use legacy palette
  }


  if (xy && !(velocity == 0 && stfu))
  {
    // MLOGD("Performance", "Set LED");
    MatrixOS::LED::SetColor(xy, palette[channel % 2][velocity], uiOpened ? canvasLedLayer : 0);
  }
  else if (stfu)
  {
    if (velocity == 0)
    { stfuMap[note] = stfu; }
    else
    { stfuMap[note] = -1; }
  }
}

vector<uint8_t> sysExBuffer;
void Performance::SysExHandler(MidiPacket midiPacket)
{
  // New SysEx, clear buffer
  if(sysExBuffer.empty())
  {
    sysExBuffer.reserve(400);
  }
  
  // Insert data to buffer
  sysExBuffer.insert(sysExBuffer.end(), midiPacket.data, midiPacket.data + midiPacket.Length());

  // If not end of sysex, return, do not parse
  if(midiPacket.status != SysExEnd)
  { return; }

  //Get rid of the 0xF7 ending byte
  sysExBuffer.pop_back(); 

  // Parsing because sysex is completed
  switch (sysExBuffer[0])
  {
    case 0x5f: //Apollo batch fill - https://github.com/mat1jaczyyy/lpp-performance-cfw/blob/0c2ec2a71030306ab7e5491bd49d72440d8c0199/src/sysex/sysex.c#L54-L120
    {
      // MLOGD("Performance", "Apollo batch Fill");
      if(sysExBuffer.size() < 5) { return; }

      uint8_t targetLayer = uiOpened ? canvasLedLayer : 0;

      uint16_t ptr = 1; // Index 0 is the command 0x5f, we start ptr at 1
      while (ptr < sysExBuffer.size())
      {
        // Extract the color data
        uint8_t colorR = (sysExBuffer[ptr] & 0x3F);
        uint8_t colorG = (sysExBuffer[ptr + 1] & 0x3F);
        uint8_t colorB = (sysExBuffer[ptr + 2] & 0x3F);
        
        // Remapped color from 6 bit to 8 bit
        colorR = (colorR << 2) + (colorR >> 4);
        colorG = (colorG << 2) + (colorG >> 4);
        colorB = (colorB << 2) + (colorB >> 4);

        // Create the color
        Color color = Color(colorR, colorG, colorB);

        // Get how many NN (Note Numbers) follows
        uint8_t n_count = ((sysExBuffer[ptr] & 0x40) >> 4) | ((sysExBuffer[ptr + 1] & 0x40) >> 5) | ((sysExBuffer[ptr + 2] & 0x40) >> 6);

        ptr += 3; //We finish reading the first 3 bit, inc the ptr by 3

        //If nums of NN is 0, then the next byte is the number of NN
        if(n_count == 0) { n_count = sysExBuffer[ptr++]; } //If all 3 bits are 0, then it's 64 (0b111111

        // MLOGD("Performance", "Color: #%.2X%.2X%.2X, NN count: %d", color.R, color.G, color.B, n_count);

        // Goes through all N
        for (uint16_t n = 0; n < n_count; n++)
        {
          if (sysExBuffer[ptr] == 0)  // Global full
          { MatrixOS::LED::Fill(color, targetLayer); }
          else if (sysExBuffer[ptr] < 99)  // Grid
          {
            Point xy = Point(sysExBuffer[ptr] % 10 - 1, 8 - (sysExBuffer[ptr] / 10));
            // MLOGD("Performance", "Grid %d %d", xy.x, xy.y);
            MatrixOS::LED::SetColor(xy, color, targetLayer);
          }
          else if (sysExBuffer[ptr] == 99)  // Mode Light
          {
            // Not implemented - Maybe a TODO
          }
          else if (sysExBuffer[ptr] < 110)  // Row Fill
          {
            int8_t row = 108 - sysExBuffer[ptr];
            for (int8_t x = 0; x < 10; x++)
            { MatrixOS::LED::SetColor(Point(x, row), color, targetLayer); }
          }
          else if (sysExBuffer[ptr] < 120)  // Column Fill
          {
            int8_t column = sysExBuffer[ptr] - 111;
            for (int8_t y = 0; y < 10; y++)
            { MatrixOS::LED::SetColor(Point(column, y), color, targetLayer); }
          }
          ptr++; //Since ptr is the pointer of in vector, we need to read the next NN, inc the ptr by 1
        }
      }
      break;
    }
    case 0x5e: //Apollo regular fill - https://github.com/mat1jaczyyy/lpp-performance-cfw/blob/0c2ec2a71030306ab7e5491bd49d72440d8c0199/src/sysex/sysex.c#L115
    {
      // MLOGD("Performance", "Apollo batch Fill");
      if(sysExBuffer.size() < 5) { return; }

      uint8_t targetLayer = uiOpened ? canvasLedLayer : 0;

      uint16_t ptr = 1; // Index 0 is the command 0x5f, we start ptr at 1
      while (ptr < sysExBuffer.size())
      {
        // Extract the color data
        uint8_t index = sysExBuffer[ptr];
        uint8_t colorR = (sysExBuffer[ptr + 1] & 0x3F);
        uint8_t colorG = (sysExBuffer[ptr + 2] & 0x3F);
        uint8_t colorB = (sysExBuffer[ptr + 3] & 0x3F);
        
        // Remapped color from 6 bit to 8 bit
        colorR = (colorR << 2) + (colorR >> 4);
        colorG = (colorG << 2) + (colorG >> 4);
        colorB = (colorB << 2) + (colorB >> 4);

        // Create the color
        Color color = Color(colorR, colorG, colorB);

        ptr += 4; //We finish reading the first 3 bit, inc the ptr by 3

        // MLOGD("Performance", "Color: #%.2X%.2X%.2X, NN count: %d", color.R, color.G, color.B, n_count);

        if (index == 0)  // Global full
        { MatrixOS::LED::Fill(color, targetLayer); }
        else if (index < 99)  // Grid
        {
          Point xy = Point(index % 10 - 1, 8 - (index / 10));
          MatrixOS::LED::SetColor(xy, color, targetLayer);
        }
        else if (index == 99)  // Mode Light
        {
          // Not implemented - Maybe a TODO
        }
        else if (index < 110)  // Row Fill
        {
          int8_t row = 108 - index;
          for (int8_t x = 0; x < 10; x++)
          { MatrixOS::LED::SetColor(Point(x, row), color, targetLayer); }
        }
        else if (index < 120)  // Column Fill
        {
          int8_t column = index - 111;
          for (int8_t y = 0; y < 10; y++)
          { MatrixOS::LED::SetColor(Point(column, y), color, targetLayer); }
        }
      }
      break;
    }
  
    default:
      break;
  }

  // Clear buffer since we are done parsing SysEx
  sysExBuffer.clear();
}

void Performance::KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo) {
  Point xy = MatrixOS::KEYPAD::ID2XY(KeyID);
  if (xy)  // IF XY is vaild, means it's on the main grid
  { GridKeyEvent(xy, keyInfo); }
  else  // XY Not vaild,
  { IDKeyEvent(KeyID, keyInfo); }
}

void Performance::GridKeyEvent(Point xy, KeyInfo* keyInfo) {
  // MLOGD("Performance Mode", "KeyEvent %d %d", xy.x, xy.y);
  int8_t note = XYToNote(xy);

  if (note == -1)
  { return; }

  if(velocitySensitive)
  {
    if (keyInfo->state == PRESSED)
    { MatrixOS::MIDI::Send(MidiPacket(0, NoteOn, 0, note, keyInfo->velocity.to7bits())); }
    else if (keyInfo->state == AFTERTOUCH)
    { MatrixOS::MIDI::Send(MidiPacket(0, AfterTouch, 0, note, keyInfo->velocity.to7bits())); }
    else if (keyInfo->state == RELEASED)
    { MatrixOS::MIDI::Send(MidiPacket(0, NoteOn, 0, note, 0)); }
  } else {
    if(keyInfo->state == PRESSED)
    { MatrixOS::MIDI::Send(MidiPacket(0, NoteOn, 0, note, 127)); }
    else if(keyInfo->state == RELEASED)
    { MatrixOS::MIDI::Send(MidiPacket(0, NoteOn, 0, note, 0)); }
  }
}

void Performance::IDKeyEvent(uint16_t keyID, KeyInfo* keyInfo) {
  // MLOGD("Performance", "Key Event");
  if (keyID == 0 && keyInfo->state == (menuLock ? HOLD : PRESSED))
  {
    // MatrixOS::LED::CopyLayer(0, canvasLedLayer);
    MatrixOS::MIDI::Send(MidiPacket(0, ControlChange, 0, 121, 0)); //For Apollo Clearing
    ActionMenu(); 
  }
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
        MatrixOS::LED::SetColor(xy, 0, 0);
      }
      stfuMap[note] = -1;
    }
  }
}

void Performance::ActionMenu() {
  MLOGD("Performance", "Enter Action Menu");

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
  UIButtonDimmable velocityToggle("Velocity Sensitive", Color(0xFFFFFF), [&]() -> bool { return velocitySensitive; }, [&]() -> void { velocitySensitive = !velocitySensitive;});
  actionMenu.AddUIComponent(velocityToggle, Point(6, 0));

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
  actionMenu.SetLoopFunc([&]() -> void {  //Keep buffer updated even when action menu is currently open
      struct MidiPacket midiPacket;
      while (MatrixOS::MIDI::Get(&midiPacket))
      { MidiEventHandler(midiPacket); }
  });

  actionMenu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool { 
    if(keyEvent->id == FUNCTION_KEY)
    {
      if(keyEvent->info.state == HOLD)
      { Exit(); }
      else if(keyEvent->info.state == RELEASED)
      { actionMenu.Exit(); }
      return true; //Block UI from to do anything with FN, basiclly this function control the life cycle of the UI
    }
    return false;
   });

  MatrixOS::LED::CopyLayer(canvasLedLayer, 0);

  uiOpened = true;
  actionMenu.Start();
  uiOpened = false;

  MLOGD("Performance", "Exit Action Menu");
}

// #endif
