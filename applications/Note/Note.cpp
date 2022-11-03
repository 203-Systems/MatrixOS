#include "Note.h"
#include "ScaleVisualizer.h"

void Note::Setup() {
  UI actionMenu("Action Menu", Color(0x00FFFF));

  ScaleVisualizer scaleVisualizer(&configs[activeConfig].rootKey, &configs[activeConfig].scale, configs[activeConfig].color, configs[activeConfig].rootColor);
  actionMenu.AddUIComponent(scaleVisualizer, Point(0, 0));

  UISelector scaleSelector(Dimension(8, 2), Color(0xD41B73), &configs[activeConfig].scale, 16, scales, scale_names);
  actionMenu.AddUIComponent(scaleSelector, Point(0, 6));

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
  actionMenu.Start();

  Exit();
}

// void Note::Loop() {
//   struct KeyEvent keyEvent;
//   while (MatrixOS::KEYPAD::Get(&keyEvent))
//   { KeyEventHandler(keyEvent.id, &keyEvent.info); }

//   // struct MidiPacket midiPacket;
//   // while (MatrixOS::MIDI::Get(&midiPacket))
//   // { MidiEventHandler(midiPacket); }
// }

// void Note::KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo) {
//   Point xy = MatrixOS::KEYPAD::ID2XY(KeyID);
//   if (xy)  // IF XY is vaild, means it's on the main grid
//   { GridKeyEvent(xy, keyInfo); }
//   else  // XY Not vaild,
//   { IDKeyEvent(KeyID, keyInfo); }
// }

// void Note::GridKeyEvent(Point xy, KeyInfo* keyInfo) {
//   // MatrixOS::Logging::LogDebug("Note Mode", "KeyEvent %d %d", xy.x, xy.y);
//   int8_t note = XYToNote(xy);

//   if (note == -1)
//   { return; }

//   if (keyInfo->state == PRESSED)
//   { MatrixOS::MIDI::Send(MidiPacket(0, NoteOn, 0, note, keyInfo->velocity.to7bits())); }
//   else if (keyInfo->state == AFTERTOUCH)
//   { MatrixOS::MIDI::Send(MidiPacket(0, AfterTouch, 0, note, keyInfo->velocity.to7bits())); }
//   else if (keyInfo->state == RELEASED)
//   { MatrixOS::MIDI::Send(MidiPacket(0, compatibilityMode ? NoteOn : NoteOff, 0, note, keyInfo->velocity.to7bits())); }
// }

// void Note::IDKeyEvent(uint16_t keyID, KeyInfo* keyInfo) {
//   // MatrixOS::Logging::LogDebug(name, "Key Event");
//   if (keyID == 0 && keyInfo->state == (menuLock ? HOLD : PRESSED))
//   { ActionMenu(); }
// }

// void Note::stfuScan() {
//   for (uint8_t note = 0; note < 128; note++)
//   {
//     if (stfuMap[note] > 0)
//     { stfuMap[note]--; }
//     else if (stfuMap[note] == 0)
//     {
//       Point xy = NoteToXY(note);
//       if (xy)
//       {
//         MatrixOS::LED::SetColor(xy, 0, canvasLedLayer);
//         MatrixOS::LED::Update(canvasLedLayer);
//       }
//       stfuMap[note] = -1;
//     }
//   }
// }

//   MatrixOS::Logging::LogDebug(name, "Enter Action Menu");

//   UI actionMenu("Action Menu", Color(0x00FFAA), true);

//   UIButtonLarge brightnessBtn(
//       "Brightness", Color(0xFFFFFF), Dimension(2, 2), [&]() -> void { MatrixOS::SYS::NextBrightness(); },
//       [&]() -> void { BrightnessControl().Start(); });
//   actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

//   // Rotation control and canvas
//   UIButtonLarge clearCanvasBtn("Clear Canvas", Color(0x00FF00), Dimension(2, 1),
//                                [&]() -> void { MatrixOS::LED::Fill(0, canvasLedLayer); });
//   actionMenu.AddUIComponent(clearCanvasBtn, Point(3, 2));

//   UIButtonLarge rotatRightBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2),
//                               [&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
//   actionMenu.AddUIComponent(rotatRightBtn, Point(5, 3));

//   UIButtonLarge rotateDownBtn("Rotate to this side", Color(0x00FF00), Dimension(2, 1),
//                               [&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
//   actionMenu.AddUIComponent(rotateDownBtn, Point(3, 5));

//   UIButtonLarge rotateLeftBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2),
//                               [&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
//   actionMenu.AddUIComponent(rotateLeftBtn, Point(2, 3));

//   // Note Pad
//   UINotePad notePad(Dimension(8, 2), keymap_color[currentKeymap], keymap_channel[currentKeymap],
//                     (uint8_t*)note_pad_map[currentKeymap]);
//   actionMenu.AddUIComponent(notePad, Point(0, 6));

//   // Other Controls
//   UIButton systemSettingBtn("System Setting", Color(0xFFFFFF), [&]() -> void { MatrixOS::SYS::OpenSetting(); });
//   actionMenu.AddUIComponent(systemSettingBtn, Point(7, 5));

//   UIButtonDimmable menuLockBtn(
//       "Menu Lock", Color(0xA0FF00), [&]() -> bool { return menuLock; }, [&]() -> void { menuLock = !menuLock; });
//   actionMenu.AddUIComponent(menuLockBtn, Point(0, 5));  // Current the currentKeymap is directly linked to
//                                                         // compatibilityMode. Do we really need > 2 keymap tho?

//   UIButtonDimmable flickerReductionBtn(
//       "Flicker Reduction", Color(0xAAFF00), [&]() -> bool { return stfu; },
//       [&]() -> void { stfu = bool(!stfu) * STFU_DEFAULT; });
//   actionMenu.AddUIComponent(flickerReductionBtn, Point(0, 0));  // Current the currentKeymap is directly linked to
//                                                                 // compatibilityMode. Do we really need > 2 keymap tho?

//   UIButtonDimmable compatibilityModeBtn(
//       "Compatibility Mode", Color(0xFFFF00), [&]() -> bool { return compatibilityMode; },
//       [&]() -> void {
//         compatibilityMode = !compatibilityMode;
//         currentKeymap = compatibilityMode;
//         notePad.SetColor(keymap_color[currentKeymap]);
//         notePad.SetChannel(keymap_channel[currentKeymap]);
//         notePad.SetMap((uint8_t*)note_pad_map[currentKeymap]);
//       });
//   actionMenu.AddUIComponent(compatibilityModeBtn, Point(7, 0));  // Current the currentKeymap is directly linked to
//                                                                  // compatibilityMode. Do we really need > 2 keymap
//                                                                  // tho?
//   actionMenu.SetLoopFunc([&]() -> void {  //Keep buffer updated even when action menu is currently open
//       struct MidiPacket midiPacket;
//       while (MatrixOS::MIDI::Get(&midiPacket))
//       { MidiEventHandler(midiPacket); }
//   });

//   actionMenu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool { 
//     if(keyEvent->id == FUNCTION_KEY)
//     {
//       if(keyEvent->info.state == HOLD)
//       { Exit(); }
//       else if(keyEvent->info.state == RELEASED)
//       { actionMenu.Exit(); }
//       return true; //Block UI from to do anything with FN, basiclly this function control the life cycle of the UI
//     }
//     return false;
//    });

//   actionMenu.Start();

//   MatrixOS::Logging::LogDebug(name, "Exit Action Menu");
// }

// // #endif
