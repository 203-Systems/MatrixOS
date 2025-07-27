#include "Strum.h"
#include "StrumChordVisualizer.h"
#include "StrumOctaveShifter.h"
#include "StrumDurationModifier.h"
#include "StrumBar.h"

void Strum::Setup() {

    // Load From NVS
    if (nvsVersion == (uint32_t)STRUM_APP_VERSION)
    { 
      MatrixOS::NVS::GetVariable(STRUM_CONFIGS_HASH, strumBarConfigs, sizeof(strumBarConfigs)); 
    }
    else
    { 
      MatrixOS::NVS::SetVariable(STRUM_CONFIGS_HASH, strumBarConfigs, sizeof(strumBarConfigs)); \
      nvsVersion = STRUM_APP_VERSION;
    }

    UI strumUI("Strum", Color(0xFFFFFF));

    StrumChordVisualizer scaleVisualizer(&strumBarConfigs[activeLayout].root, (uint16_t*)&strumBarConfigs[activeLayout].chord, Color(0x8000FF), Color(0xFF00FF));
    strumUI.AddUIComponent(scaleVisualizer, Point(0, 0));

    StrumOctaveShifter octaveShifter(8, (uint8_t*)&strumBarConfigs[activeLayout].octave);
    strumUI.AddUIComponent(octaveShifter, Point(0, 2));

    StrumDurationModifier durationModifier(8, (uint16_t*)&strumBarConfigs[activeLayout].note_length);
    strumUI.AddUIComponent(durationModifier, Point(0, 3));

    // UIItemSelector majorChordSelectorBar(Dimension(7, 1), Color(0xFF0000), &chord, 7, major_chords, major_chord_names);
    // strumUI.AddUIComponent(majorChordSelectorBar, Point(0, 4));

    // UIItemSelector minorChordSelectorBar(Dimension(7, 1), Color(0xFF6A00), &chord, 7, minor_chords, minor_chord_names);
    // strumUI.AddUIComponent(minorChordSelectorBar, Point(0, 5));

    // UIItemSelector dominantChordSelectorBar(Dimension(4, 1), Color(0xFFBF00), &chord, 4, dominant_chords, dominant_chord_names);
    // strumUI.AddUIComponent(dominantChordSelectorBar, Point(0, 6));

    // UIItemSelector alteredChordSelectorBar(Dimension(4, 1), Color(0xFFFF00), &chord, 4, altered_chords, altered_chord_names);
    // strumUI.AddUIComponent(alteredChordSelectorBar, Point(4, 6));

    // UIItemSelector addChordSelectorBar(Dimension(2, 1), Color(0xAAFF00), &chord, 2, add_chords, add_chord_names);
    // strumUI.AddUIComponent(addChordSelectorBar, Point(0, 7));

    // UIItemSelector suspendedChordSelectorBar(Dimension(2, 1), Color(0x2BFF00), &chord, 2, suspended_chords, suspended_chord_names);
    // strumUI.AddUIComponent(suspendedChordSelectorBar, Point(2, 7));

    // UIItemSelector diminishedChordSelectorBar(Dimension(2, 1), Color(0x00FF95), &chord, 2, diminished_chords, diminished_chord_names);
    // strumUI.AddUIComponent(diminishedChordSelectorBar, Point(4, 7));

    // UIItemSelector augmentedChordSelectorBar(Dimension(2, 1), Color(0x00AAFF), &chord, 2, augmented_chords, augmented_chord_names);
    // strumUI.AddUIComponent(augmentedChordSelectorBar, Point(6, 7));

    UIButton majorBtn;
    majorBtn.SetName("Major Chord");
    majorBtn.SetColorFunc([&]() -> Color { return Color(0xFF0000).DimIfNot(strumBarConfigs[activeLayout].chord == EChord::MAJOR_TRIAD); });
    majorBtn.OnPress([&]() -> void { strumBarConfigs[activeLayout].chord = EChord::MAJOR_TRIAD; });
    strumUI.AddUIComponent(majorBtn, Point(0, 7));

    UIButton minorBtn;
    minorBtn.SetName("Minor Chord");
    minorBtn.SetColorFunc([&]() -> Color { return Color(0xFF6A00).DimIfNot(strumBarConfigs[activeLayout].chord == EChord::MINOR_TRIAD); });
    minorBtn.OnPress([&]() -> void { strumBarConfigs[activeLayout].chord = EChord::MINOR_TRIAD; });
    strumUI.AddUIComponent(minorBtn, Point(1, 7));

    UIButton major7thBtn;
    major7thBtn.SetName("Major 7th Chord");
    major7thBtn.SetColorFunc([&]() -> Color { return Color(0xFFBF00).DimIfNot(strumBarConfigs[activeLayout].chord == EChord::MAJOR_SEVENTH); });
    major7thBtn.OnPress([&]() -> void { strumBarConfigs[activeLayout].chord = EChord::MAJOR_SEVENTH; });
    strumUI.AddUIComponent(major7thBtn, Point(3, 7));

    UIButton minor7thBtn;
    minor7thBtn.SetName("Minor 7th Chord");
    minor7thBtn.SetColorFunc([&]() -> Color { return Color(0xFFFF00).DimIfNot(strumBarConfigs[activeLayout].chord == EChord::MINOR_SEVENTH); });
    minor7thBtn.OnPress([&]() -> void { strumBarConfigs[activeLayout].chord = EChord::MINOR_SEVENTH; });
    strumUI.AddUIComponent(minor7thBtn, Point(4, 7));


    UIButton augmentedBtn;
    augmentedBtn.SetName("Augmented Chord");
    augmentedBtn.SetColorFunc([&]() -> Color { return Color(0x00FF95).DimIfNot(strumBarConfigs[activeLayout].chord == EChord::AUGMENTED); });
    augmentedBtn.OnPress([&]() -> void { strumBarConfigs[activeLayout].chord = EChord::AUGMENTED; });
    strumUI.AddUIComponent(augmentedBtn, Point(6, 7));

    UIButton diminishedBtn;
    diminishedBtn.SetName("Diminished Chord");
    diminishedBtn.SetColorFunc([&]() -> Color { return Color(0x00AAFF).DimIfNot(strumBarConfigs[activeLayout].chord == EChord::DIMINISHED); });
    diminishedBtn.OnPress([&]() -> void { strumBarConfigs[activeLayout].chord = EChord::DIMINISHED; });
    strumUI.AddUIComponent(diminishedBtn, Point(7, 7));

    StrumBar strumBarL(&strumBarConfigs[0]);
    strumUI.AddUIComponent(strumBarL, Point(-1, 0));

    // StrumBar strumBarR(&strumBarConfigs[1]);
    StrumBar strumBarR(&strumBarConfigs[0]); // Keep as same for now
    strumUI.AddUIComponent(strumBarR, Point(8, 0));
    
    strumUI.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == PRESSED)
      {
        ActionMenu();
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
        MatrixOS::NVS::SetVariable(STRUM_CONFIGS_HASH, strumBarConfigs, sizeof(strumBarConfigs));
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