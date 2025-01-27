#include "Strum.h"
#include "StrumScaleVisualizer.h"
#include "StrumOctaveShifter.h"
#include "StrumBar.h"

void Strum::Setup() {
    UI strumUI("Strum", Color(0xFFFFFF));

    StrumScaleVisualizer scaleVisualizer(&root, (uint16_t*)&chord, Color(0x8000FF), Color(0xFF00FF));
    strumUI.AddUIComponent(scaleVisualizer, Point(0, 0));

    StrumOctaveShifter octaveShifter(8, &octave);
    strumUI.AddUIComponent(octaveShifter, Point(0, 2));

    UIItemSelector majorChordSelectorBar(Dimension(7, 1), Color(0xFF0000), &chord, 7, major_chords, major_chord_names);
    strumUI.AddUIComponent(majorChordSelectorBar, Point(0, 4));

    UIItemSelector minorChordSelectorBar(Dimension(7, 1), Color(0xFF6A00), &chord, 7, minor_chords, minor_chord_names);
    strumUI.AddUIComponent(minorChordSelectorBar, Point(0, 5));

    UIItemSelector dominantChordSelectorBar(Dimension(4, 1), Color(0xFFBF00), &chord, 4, dominant_chords, dominant_chord_names);
    strumUI.AddUIComponent(dominantChordSelectorBar, Point(0, 6));

    UIItemSelector alteredChordSelectorBar(Dimension(4, 1), Color(0xFFFF00), &chord, 4, altered_chords, altered_chord_names);
    strumUI.AddUIComponent(alteredChordSelectorBar, Point(4, 6));

    UIItemSelector addChordSelectorBar(Dimension(2, 1), Color(0xAAFF00), &chord, 2, add_chords, add_chord_names);
    strumUI.AddUIComponent(addChordSelectorBar, Point(0, 7));

    UIItemSelector suspendedChordSelectorBar(Dimension(2, 1), Color(0x2BFF00), &chord, 2, suspended_chords, suspended_chord_names);
    strumUI.AddUIComponent(suspendedChordSelectorBar, Point(2, 7));

    UIItemSelector diminishedChordSelectorBar(Dimension(2, 1), Color(0x00FF95), &chord, 2, diminished_chords, diminished_chord_names);
    strumUI.AddUIComponent(diminishedChordSelectorBar, Point(4, 7));

    UIItemSelector augmentedChordSelectorBar(Dimension(2, 1), Color(0x00AAFF), &chord, 2, augmented_chords, augmented_chord_names);
    strumUI.AddUIComponent(augmentedChordSelectorBar, Point(6, 7));

    StrumBar strumBarL(&chord, &root, &octave, &midi_channel, Color(0x00FFFF));
    strumUI.AddUIComponent(strumBarL, Point(-1, 0));

    StrumBar strumBarR(&chord, &root, &octave, &midi_channel, Color(0x00FFFF));
    strumUI.AddUIComponent(strumBarR, Point(8, 0));
    
    strumUI.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == PRESSED)
      {
        ActionMenu();
        root.Save();
        chord.Save();
        octave.Save();
      }
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });

    strumUI.Start();
    Exit();
}


void Strum::ActionMenu() {
  UI actionMenu("Action Menu", Color(0x00FF00), true);

  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([&]() -> void { MatrixOS::LED::NextBrightness(); });
  brightnessBtn.OnHold([&]() -> void { BrightnessControl().Start(); });
  actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

  UIButton rotateUpBtn;
  rotateUpBtn.SetName("This Does Nothing");
  rotateUpBtn.SetColor(Color(0x00FF00));
  rotateUpBtn.SetSize(Dimension(2, 1));
  actionMenu.AddUIComponent(rotateUpBtn, Point(3, 2));

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

UIButton systemSettingBtn;
  systemSettingBtn.SetName("System Setting");
  systemSettingBtn.SetColor(Color(0xFFFFFF));
  systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });
  actionMenu.AddUIComponent(systemSettingBtn, Point(7, 7));

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

  actionMenu.Start();
}

void Strum::Loop() {
    // Do nothing
}