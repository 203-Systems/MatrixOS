#include "Performance8x8.h"

void Performance::Setup() {
  // Load variable
  canvasLedLayer = MatrixOS::LED::CurrentLayer();
  currentKeymap = 0;

  MatrixOS::NVS::GetVariable(custom_palette_available_nvs_hash, custom_palette_available, sizeof(custom_palette_available));
  for (uint8_t i = 0; i < CUSTOM_PALETTE_COUNT; i++)
  {
    if (custom_palette_available[i])
    {
      MatrixOS::NVS::GetVariable(custom_palette_nvs_hash[i], custom_palette[i], sizeof(custom_palette[i]));
    }
    else
    {
      custom_palette[i][0] = Color(0);
      for (uint8_t j = 1; j < 128; j++)
      {
        custom_palette[i][j] = Color(0xFFFFFF);
      }
    }
  }
}

void Performance::Loop() {
  if (stfu && stfuTimer.Tick(1000 / Device::fps))
  {
    stfuScan();
  }

  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  {
    KeyEventHandler(keyEvent.id, &keyEvent.info);
  }

  struct MidiPacket midiPacket;
  while (MatrixOS::MIDI::Get(&midiPacket))
  {
    MidiEventHandler(midiPacket);
  }
}

void Performance::MidiEventHandler(MidiPacket midiPacket) {
  // MLOGD("Performance", "Midi Received from %d - %d %d %d %d", midiPacket.port, midiPacket.status, midiPacket.data[0],
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
      {
        return Point(8, note - 100);
      }
      else if (note > 115 && note < 124)  // Side Light Bottom Row
      {
        return Point(note - 116, 8);
      }
      else if (note > 107 && note < 116)  // Side Light Left Column
      {
        return Point(-1, note - 108);
      }
      else if (note > 27 && note < 36)  // Side Light Top Row
      {
        return Point(note - 28, -1);
      }
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
int8_t Performance::XYToNote(Point xy, bool combo_key) {
  switch (currentKeymap)
  {
    case 0:
    {
      if(combo_key && xy.x == 0 && xy.y >= 0 && xy.y < 8) // Combo map left
      {
        return touch_keymap[currentKeymap][3][xy.y];
      }
      else if(combo_key && xy.x == 7 && xy.y >= 0 && xy.y < 8)// Combo map right
      {
        return touch_keymap[currentKeymap][1][xy.y];
      }
      else if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)
      {
        return keymap[currentKeymap][xy.y][xy.x];
      }
      else if (xy.y == -1 && xy.x >= 0 && xy.x < 8)  // TouchBar Top Row
      {
        return touch_keymap[currentKeymap][0][xy.x];
      }
      else if (xy.x == 8 && xy.y >= 0 && xy.y < 8)  // TouchBar Right Column
      {
        return touch_keymap[currentKeymap][1][xy.y];
      }
      else if (xy.y == 8 && xy.x >= 0 && xy.x < 8)  // TouchBar Bottom Row
      {
        return touch_keymap[currentKeymap][2][xy.x];
      }
      else if (xy.x == -1 && xy.y >= 0 && xy.y < 8)  // TouchBar Left Column
      {
        return touch_keymap[currentKeymap][3][xy.y];
      }
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
  // MLOGD("Performance", "Midi Received %#02X %#02X %#02X", channel, note, velocity);
  Point xy = NoteToXY(note);

  if (xy && !(velocity == 0 && stfu))
  {
    // MLOGD("Performance", "Set LED");
    Color color;
    if (channel < BUILTIN_PALETTE_COUNT)
    {
      color = palette[channel][velocity];
      MatrixOS::LED::SetColor(xy, color, uiOpened ? canvasLedLayer : 0);
    }
    else if (channel < BUILTIN_PALETTE_COUNT + CUSTOM_PALETTE_COUNT)
    {
      color = custom_palette[channel - BUILTIN_PALETTE_COUNT][velocity];
      MatrixOS::LED::SetColor(xy, color, uiOpened ? canvasLedLayer : 0);
    }
  }
  else if (stfu)
  {
    if (velocity == 0)
    {
      stfuMap[note] = stfu;
    }
    else
    {
      stfuMap[note] = -1;
    }
  }
}

void Performance::SysExHandler(MidiPacket midiPacket) {
  // New SysEx, clear buffer
  if (sysExBuffer.empty())
  {
    sysExBuffer.reserve(400);
  }

  // Insert data to buffer
  sysExBuffer.insert(sysExBuffer.end(), midiPacket.data, midiPacket.data + midiPacket.Length());

  // If not end of sysex, return, do not parse
  if (midiPacket.status != SysExEnd)
  {
    return;
  }

  // Get rid of the 0xF7 ending byte
  sysExBuffer.pop_back();

  // Parsing because sysex is completed
  switch (sysExBuffer[0])
  {
    case 0x5f:  // Apollo batch fill - https://github.com/mat1jaczyyy/lpp-performance-cfw/blob/0c2ec2a71030306ab7e5491bd49d72440d8c0199/src/sysex/sysex.c#L54-L120
    {
      // MLOGD("Performance", "Apollo batch Fill");
      if (sysExBuffer.size() < 5)
      {
        return;
      }

      uint8_t targetLayer = uiOpened ? canvasLedLayer : 0;

      uint16_t ptr = 1;  // Index 0 is the command 0x5f, we start ptr at 1
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

        ptr += 3;  // We finish reading the first 3 bit, inc the ptr by 3

        // If nums of NN is 0, then the next byte is the number of NN
        if (n_count == 0)
        {
          n_count = sysExBuffer[ptr++];
        }  // If all 3 bits are 0, then it's 64 (0b111111

        // MLOGD("Performance", "Color: #%.2X%.2X%.2X, NN count: %d", color.R, color.G, color.B, n_count);

        // Goes through all N
        for (uint16_t n = 0; n < n_count; n++)
        {
          if (sysExBuffer[ptr] == 0)  // Global full
          {
            MatrixOS::LED::Fill(color, targetLayer);
          }
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
            {
              MatrixOS::LED::SetColor(Point(x, row), color, targetLayer);
            }
          }
          else if (sysExBuffer[ptr] < 120)  // Column Fill
          {
            int8_t column = sysExBuffer[ptr] - 111;
            for (int8_t y = 0; y < 10; y++)
            {
              MatrixOS::LED::SetColor(Point(column, y), color, targetLayer);
            }
          }
          ptr++;  // Since ptr is the pointer of in vector, we need to read the next NN, inc the ptr by 1
        }
      }
      break;
    }
    case 0x5e:  // Apollo regular fill - https://github.com/mat1jaczyyy/lpp-performance-cfw/blob/0c2ec2a71030306ab7e5491bd49d72440d8c0199/src/sysex/sysex.c#L115
    {
      // MLOGD("Performance", "Apollo batch Fill");
      if (sysExBuffer.size() < 5)
      {
        return;
      }

      uint8_t targetLayer = uiOpened ? canvasLedLayer : 0;

      uint16_t ptr = 1;  // Index 0 is the command 0x5f, we start ptr at 1
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

        ptr += 4;  // We finish reading the first 3 bit, inc the ptr by 3

        // MLOGD("Performance", "Color: #%.2X%.2X%.2X, NN count: %d", color.R, color.G, color.B, n_count);

        if (index == 0)  // Global full
        {
          MatrixOS::LED::Fill(color, targetLayer);
        }
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
          {
            MatrixOS::LED::SetColor(Point(x, row), color, targetLayer);
          }
        }
        else if (index < 120)  // Column Fill
        {
          int8_t column = index - 111;
          for (int8_t y = 0; y < 10; y++)
          {
            MatrixOS::LED::SetColor(Point(column, y), color, targetLayer);
          }
        }
      }
      break;
    }
    case 0x41:  // Retina Custom Palette
    {
      if (sysExBuffer[1] == 0x7B)  // Uploading Start
      {
        MLOGD("Performance", "Retina Custom Palette Uploading Start");
        // I really don't think I have to do anything here. I want to set custom_palette_available to false, but I don't think it's necessary & we don't know which palette is gonna
        // be write to
      }
      else if (sysExBuffer[1] == 0x3D)  // Uploading Write
      {
        MLOGD("Performance", "Retina Custom Palette Uploading Write");
        uint8_t palette_to_write = sysExBuffer[2];
        // Read 4 byte at once
        for (uint16_t i = 3; i < sysExBuffer.size(); i += 4)
        {
          uint8_t index = sysExBuffer[i];
          uint8_t colorR = (sysExBuffer[i + 1] & 0x3F);
          uint8_t colorG = (sysExBuffer[i + 2] & 0x3F);
          uint8_t colorB = (sysExBuffer[i + 3] & 0x3F);

          // Remapped color from 6 bit to 8 bit
          colorR = (colorR << 2) + (colorR >> 4);
          colorG = (colorG << 2) + (colorG >> 4);
          colorB = (colorB << 2) + (colorB >> 4);

          custom_palette[palette_to_write][index] = Color(colorR, colorG, colorB);

          MLOGD("Performance", "Custom Palette %d Index %d Color %d %d %d", palette_to_write, index, colorR, colorG, colorB);
        }
        custom_palette_available[palette_to_write] = true;
      }
      else if (sysExBuffer[1] == 0x7D)  // Uploading End
      {
        MLOGD("Performance", "Retina Custom Palette Uploading End");
        for (uint8_t i = 0; i < CUSTOM_PALETTE_COUNT; i++)
        {
          if (custom_palette_available[i])
          {
            MatrixOS::NVS::SetVariable(custom_palette_nvs_hash[i], custom_palette[i], sizeof(custom_palette[i]));
          }
        }
        MatrixOS::NVS::SetVariable(custom_palette_available_nvs_hash, custom_palette_available, sizeof(custom_palette_available));
      }
    }
    default:
      break;
  }

  // Clear buffer since we are done parsing SysEx
  sysExBuffer.clear();
}

void Performance::KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo) {
  Point xy = MatrixOS::KEYPAD::ID2XY(KeyID);
  if (xy)  // IF XY is valid, means it's on the main grid
  {
    GridKeyEvent(xy, keyInfo);
  }
  else  // XY Not valid,
  {
    IDKeyEvent(KeyID, keyInfo);
  }
}

void Performance::GridKeyEvent(Point xy, KeyInfo* keyInfo) {
  // MLOGD("Performance Mode", "KeyEvent %d %d", xy.x, xy.y);

  bool combo_key = false;
  
  // Disable Touchbar for combo key mode
  if(touch_combo_key && !(xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8))
  {
    return;
  }

  // Check if combo key should be used
  if(touch_combo_key && (xy.x == 0 || xy.x == 7) && xy.y >= 0 && xy.y < 8)
  {
    uint16_t combo_key_id = xy.y + (xy.x == 7) * 8;

    combo_key = was_combo_key & (1 << combo_key_id);

    if(combo_key == false) // if this key was not in combo key state
    {
      // Check if touchbar is touched
      for (uint8_t i = 0; i < 32; i++)
      {
        Point point;
        if(i < 8)
        {
          point = Point(i, -1);
        }
        else if(i < 16)
        {
          point = Point(8, i - 8);
        }
        else if(i < 24)
        {
          point = Point(i - 16, 8);
        }
        else
        {
          point = Point(-1, i - 24);
        }

        KeyInfo* touchKey = MatrixOS::KEYPAD::GetKey(point);
        if(touchKey != nullptr && touchKey->active())
        {
          combo_key = true;
          break;
        }
      }
    }

    // If we are in combo key state and key is pressed, set the combo key state
    if(combo_key && keyInfo->state == PRESSED)
    {
        was_combo_key |= 1 << combo_key_id;
    }

    // If we was in combo key state and key is released, clear the combo key state
    if(keyInfo->state == RELEASED)
    {
      was_combo_key &= ~(1 << combo_key_id);
    }
  }

  int8_t note = XYToNote(xy, combo_key);

  if (note == -1)
  {
    return;
  }

  if (!velocitySensitive)
  {
    if (keyInfo->state == AFTERTOUCH)
    {
      return;
    };
    if (keyInfo->velocity > 0)
    {
      keyInfo->velocity = FRACT16_MAX;
    };
  }

  if (keyInfo->state == PRESSED)
  {
    MatrixOS::MIDI::Send(MidiPacket(MIDI_PORT_EACH_CLASS, NoteOn, 0, note, keyInfo->velocity.to7bits()));
  }
  else if (keyInfo->state == AFTERTOUCH)
  {
    MatrixOS::MIDI::Send(MidiPacket(MIDI_PORT_EACH_CLASS, AfterTouch, 0, note, keyInfo->velocity.to7bits()));
  }
  else if (keyInfo->state == RELEASED)
  {
    MatrixOS::MIDI::Send(MidiPacket(MIDI_PORT_EACH_CLASS, NoteOn, 0, note, 0));
  }
}

void Performance::IDKeyEvent(uint16_t keyID, KeyInfo* keyInfo) {
  // MLOGD("Performance", "Key Event");
  if (keyID == 0 && keyInfo->state == (menuLock ? HOLD : PRESSED))
  {
    MatrixOS::MIDI::Send(MidiPacket(MIDI_PORT_EACH_CLASS, ControlChange, 0, 121, 127)); 
    MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, ControlChange, 0, 123, 0)); // All notes off
    ActionMenu();
    MatrixOS::MIDI::Send(MidiPacket(MIDI_PORT_EACH_CLASS, ControlChange, 0, 121, 0));  // For Apollo Clearing
    MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, ControlChange, 0, 123, 0)); // All notes off
  }
}

void Performance::stfuScan() {
  for (uint8_t note = 0; note < 128; note++)
  {
    if (stfuMap[note] > 0)
    {
      stfuMap[note]--;
    }
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

void Performance::PaletteViewer(uint8_t custom_palette_id) {
  MLOGD("Performance", "Custom Palette Viewer %d", custom_palette_id);

  bool modified = false;
  Timer timer;

  for (uint8_t i = 0; i < 2; i++)
  {
    MatrixOS::LED::Fade();
    while (true)
    {
      if(timer.Tick(1000/Device::fps))
      {
        MatrixOS::LED::Fill(0);
        for (uint8_t y = 0; y < 8; y++)
        {
          for (uint8_t x = 0; x < 8; x++)
          {
            uint8_t id = y * 8 + x + i * 64;
            MatrixOS::LED::SetColor(Point(x, y), custom_palette[custom_palette_id][id]);
          }
        }
        MatrixOS::LED::Update();
      }
      
      struct KeyEvent keyEvent;
      if (MatrixOS::KEYPAD::Get(&keyEvent))
      {
        Point xy = MatrixOS::KEYPAD::ID2XY(keyEvent.id);
        if(xy && xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)
        {
          uint8_t id = xy.y * 8 + xy.x + i * 64;
          if (keyEvent.info.state == RELEASED)
          {
            uint32_t old_color = custom_palette[custom_palette_id][id].RGB();
            MatrixOS::UIUtility::ColorPicker(custom_palette[custom_palette_id][id]);
            if (old_color != custom_palette[custom_palette_id][id].RGB())
            {
              MatrixOS::LED::SetColor(Point(xy.x, xy.y), custom_palette[custom_palette_id][id]);
              MatrixOS::LED::Update();
              modified = true;
            }
            
          }
          else if (keyEvent.info.state == HOLD)
          {
            string text = "Color " + std::to_string(id);
            MatrixOS::UIUtility::TextScroll(text, Color(0xFFFFFF));
          }
          else if (keyEvent.info.state == RELEASED)
          {
            break;
          }
        }
        if (keyEvent.id == FUNCTION_KEY)
        {
          if (keyEvent.info.state == HOLD)
          { 
            i = 100;  // Force exit, but still runs the saving routine
            break;
          }
          else if (keyEvent.info.state == RELEASED)
          {
            break;
          }
        }
        
      }
    }
  }

  if (modified)
  {
    custom_palette_available[custom_palette_id] = true;
    MatrixOS::NVS::SetVariable(custom_palette_nvs_hash[custom_palette_id], custom_palette[custom_palette_id], sizeof(custom_palette[custom_palette_id]));
    MatrixOS::NVS::SetVariable(custom_palette_available_nvs_hash, custom_palette_available, sizeof(custom_palette_available));
  }

  MatrixOS::LED::Fade();
  MatrixOS::LED::Fill(0);
  return;
}

void Performance::ActionMenu() {
  MLOGD("Performance", "Enter Action Menu");

  UI actionMenu("Action Menu", Color(0x00FFAA), true);

  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([&]() -> void { MatrixOS::LED::NextBrightness(); });
  brightnessBtn.OnHold([&]() -> void { BrightnessControl().Start(); });
  actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

  UIButton clearCanvasBtn;
  clearCanvasBtn.SetName("Clear Canvas");
  clearCanvasBtn.SetColor(Color(0x00FF00));
  clearCanvasBtn.SetSize(Dimension(2, 1));
  clearCanvasBtn.OnPress([&]() -> void { MatrixOS::LED::Fill(0, canvasLedLayer); });
  actionMenu.AddUIComponent(clearCanvasBtn, Point(3, 2));

  UIButton rotateRightBtn;
  rotateRightBtn.SetName("Rotate to this side");
  rotateRightBtn.SetColor(Color(0x00FF00));
  rotateRightBtn.SetSize(Dimension(1, 2));
  rotateRightBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  actionMenu.AddUIComponent(rotateRightBtn, Point(5, 3));

  UIButton rotateDownBtn;
  rotateDownBtn.SetName("Rotate to this side");
  rotateDownBtn.SetColor(Color(0x00FF00));
  rotateDownBtn.SetSize(Dimension(2, 1));
  rotateDownBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  actionMenu.AddUIComponent(rotateDownBtn, Point(3, 5));

  UIButton rotateLeftBtn;
  rotateLeftBtn.SetName("Rotate to this side");
  rotateLeftBtn.SetColor(Color(0x00FF00));
  rotateLeftBtn.SetSize(Dimension(1, 2));
  rotateLeftBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  actionMenu.AddUIComponent(rotateLeftBtn, Point(2, 3));

  // Note Pad
  UINotePad notePad(Dimension(8, 2), keymap_color[currentKeymap], keymap_channel[currentKeymap], (uint8_t*)note_pad_map[currentKeymap], velocitySensitive);
  actionMenu.AddUIComponent(notePad, Point(0, 6));

  // Other Controls
  UIToggle velocityToggle;
  velocityToggle.SetName("Velocity Sensitive");
  velocityToggle.SetColor(Color(0xFFFFFF));
  velocityToggle.SetValue(&velocitySensitive);
  velocityToggle.OnPress([&]() -> void {velocitySensitive.Save();});
  velocityToggle.SetEnabled(Device::KeyPad::velocity_sensitivity);
  actionMenu.AddUIComponent(velocityToggle, Point(7, 0));

  UIButton systemSettingBtn;
  systemSettingBtn.SetName("System Setting");
  systemSettingBtn.SetColor(Color(0xFFFFFF));
  systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });
  actionMenu.AddUIComponent(systemSettingBtn, Point(7, 5));

  UIToggle menuLockToggle;
  menuLockToggle.SetName("Menu Lock");
  menuLockToggle.SetColor(Color(0xFF0000));
  menuLockToggle.SetValue(&menuLock);
  menuLockToggle.OnPress([&]() -> void { menuLock.Save(); });
  actionMenu.AddUIComponent(menuLockToggle, Point(0, 5));

  UIToggle comboKeyToggle;
  comboKeyToggle.SetName("Touch Combo Key");
  comboKeyToggle.SetColor(Color(0x60FF00));
  comboKeyToggle.SetValue(&touch_combo_key);
  comboKeyToggle.OnPress([&]() -> void { touch_combo_key.Save(); });
  actionMenu.AddUIComponent(comboKeyToggle, Point(0, 4));

  UIButton flickerReductionToggle;
  flickerReductionToggle.SetName("Flicker Reduction");
  flickerReductionToggle.SetColorFunc([&]() -> Color { return Color(0xAAFF00).DimIfNot(stfu); });
  flickerReductionToggle.OnPress([&]() -> void { stfu = (!stfu) * STFU_DEFAULT; }); // The UIToggle already flip the value for us
  flickerReductionToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll(flickerReductionToggle.GetName() + " " + (stfu ? "On" : "Off"), flickerReductionToggle.GetColor()); });
  actionMenu.AddUIComponent(flickerReductionToggle, Point(0, 0));

  UIButton customPaletteViewer1;
  customPaletteViewer1.SetName("Custom Palette 1");
  customPaletteViewer1.SetColorFunc([&]() -> Color { return custom_palette_available[0] ? Color(0x00FFFF) : Color(0xFFFFFF).Dim(); });
  customPaletteViewer1.OnPress([&]() -> void { PaletteViewer(0); });
  actionMenu.AddUIComponent(customPaletteViewer1, Point(2, 0));

  UIButton customPaletteViewer2;
  customPaletteViewer2.SetName("Custom Palette 2");
  customPaletteViewer2.SetColorFunc([&]() -> Color { return custom_palette_available[1] ? Color(0x00FFFF) : Color(0xFFFFFF).Dim(); });
  customPaletteViewer2.OnPress([&]() -> void { PaletteViewer(1); });
  actionMenu.AddUIComponent(customPaletteViewer2, Point(3, 0));

  UIButton customPaletteViewer3;
  customPaletteViewer3.SetName("Custom Palette 3");
  customPaletteViewer3.SetColorFunc([&]() -> Color { return custom_palette_available[2] ? Color(0x00FFFF) : Color(0xFFFFFF).Dim(); });
  customPaletteViewer3.OnPress([&]() -> void { PaletteViewer(2); });
  actionMenu.AddUIComponent(customPaletteViewer3, Point(4, 0));

  UIButton customPaletteViewer4;
  customPaletteViewer4.SetName("Custom Palette 4");
  customPaletteViewer4.SetColorFunc([&]() -> Color { return custom_palette_available[3] ? Color(0x00FFFF) : Color(0xFFFFFF).Dim(); });
  customPaletteViewer4.OnPress([&]() -> void { PaletteViewer(3); });
  actionMenu.AddUIComponent(customPaletteViewer4, Point(5, 0));

  actionMenu.SetLoopFunc([&]() -> void {  // Keep buffer updated even when action menu is currently open
    struct MidiPacket midiPacket;
    while (MatrixOS::MIDI::Get(&midiPacket))
    {
      MidiEventHandler(midiPacket);
    }
  });

  actionMenu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == HOLD)
      {
        Exit();
      }
      else if (keyEvent->info.state == RELEASED)
      {
        actionMenu.Exit();
      }
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });

  MatrixOS::LED::CopyLayer(canvasLedLayer, 0);

  uiOpened = true;
  actionMenu.Start();
  uiOpened = false;

  MatrixOS::LED::Update();

  MLOGD("Performance", "Exit Action Menu");
}

// #endif
