#include "Note.h"
#include "ScaleVisualizer.h"
#include "ScaleModifier.h"
#include "UnderglowLight.h"
#include "NoteControlBar.h"
#include "ArpDirVisualizer.h"
#include "RhythmVisualizer.h"
#include "InfDisplay.h"

void Note::Setup(const vector<string>& args) {
  // Set up / Load configs --------------------------------------------------------------------------

  // Default Values
  notePadConfigs[1].color = Color(0xFF00FF);
  notePadConfigs[1].rootColor = Color(0x8000FF);

  // Load From NVS
  if (nvsVersion == (uint32_t)NOTE_APP_VERSION)
  {
    size_t storedSize = MatrixOS::NVS::GetSize(NOTE_CONFIGS_HASH);

    // Check if stored size matches current structure size
    if (storedSize == sizeof(notePadConfigs)) {
      MatrixOS::NVS::GetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs));
    } else {
      // Size mismatch - structure has changed, use defaults and save them
      MLOGD("Note", "Config size mismatch: stored=%d, expected=%d. Using defaults.", storedSize, sizeof(notePadConfigs));
      SaveConfigs();
    }
  }
  else
  {
    SaveConfigs();
    nvsVersion = NOTE_APP_VERSION;
  }

  // Load custom scales from NVS
  MatrixOS::NVS::GetVariable(CUSTOM_SCALES_HASH, custom_scales, sizeof(custom_scales));

  // Initialize runtimes
  for(uint8_t i = 0; i < 2; i++)
  {
    runtimes[i].config = &notePadConfigs[i];
    runtimes[i].arpeggiator = Arpeggiator(&runtimes[i].config->arpConfig);
    runtimes[i].midiPipeline.AddEffect("NoteLatch", &runtimes[i].noteLatch);
    runtimes[i].noteLatch.SetEnabled(false);
    runtimes[i].midiPipeline.AddEffect("ChordEffect", &runtimes[i].chordEffect);
    runtimes[i].midiPipeline.AddEffect("Arpeggiator", &runtimes[i].arpeggiator);
  }

  int32_t octaveAbs = (int32_t)abs(notePadConfigs[activeConfig].octave);

  // Set up the Action Menu UI ---------------------------------------------------------------------
  UI actionMenu("Action Menu", Color(0x00FFFF), false);
  bool configModified = false;

  // Note Pad Control
  UIButton scaleSelectorBtn;
  scaleSelectorBtn.SetName("Scale Selector");
  scaleSelectorBtn.SetColor(Color(0xFF0090));
  scaleSelectorBtn.OnPress([&]() -> void { ScaleSelector(); });
  actionMenu.AddUIComponent(scaleSelectorBtn, Point(7, 2));

  UIButton layoutSelectorBtn;
  layoutSelectorBtn.SetName("Layout Selector");
  layoutSelectorBtn.SetColor(Color(0xFFFF00));
  layoutSelectorBtn.OnPress([&]() -> void { LayoutSelector(); });
  actionMenu.AddUIComponent(layoutSelectorBtn, Point(7, 3));

  UIButton channelSelectorBtn;
  channelSelectorBtn.SetName("Channel Selector");
  channelSelectorBtn.SetColor(Color(0x60FF00));
  channelSelectorBtn.OnPress([&]() -> void { ChannelSelector(); });
  actionMenu.AddUIComponent(channelSelectorBtn, Point(7, 4));

  UIButton forceSensitiveToggle;
  forceSensitiveToggle.SetName("Velocity Sensitive");
  if(Device::KeyPad::velocity_sensitivity)
  {
    forceSensitiveToggle.SetColorFunc([&]() -> Color { return  Color(0x00FFB0).DimIfNot(notePadConfigs[activeConfig.Get()].forceSensitive); });
    forceSensitiveToggle.OnPress([&]() -> void { notePadConfigs[activeConfig.Get()].forceSensitive = !notePadConfigs[activeConfig.Get()].forceSensitive; configModified = true; });
    forceSensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll(forceSensitiveToggle.GetName() + " " + (notePadConfigs[activeConfig.Get()].forceSensitive ? "On" : "Off"), forceSensitiveToggle.GetColor()); });
  }
  else
  {
    forceSensitiveToggle.SetColor(Color(0x00FFB0).Dim());
    forceSensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll("Velocity Sensitivity Not Supported", Color(0x00FFB0)); });
  }
  actionMenu.AddUIComponent(forceSensitiveToggle, Point(7, 5));

  // Split View
  UIButton splitViewToggle;
  splitViewToggle.SetName("Split View");
  splitViewToggle.SetColorFunc([&]() -> Color { 
    switch(splitView)
    {

      case VERT_SPLIT: return Color(0x00FFFF);
      case HORIZ_SPLIT: return Color(0xFF00FF);
      default: return Color::White.Dim();
    }
  });
  splitViewToggle.OnPress([&]() -> void { splitView = (ESpiltView)(((uint8_t)splitView + 1) % 3); });
  splitViewToggle.OnHold([&]() -> void { 
    switch(splitView)
    {
      case SINGLE_VIEW: MatrixOS::UIUtility::TextScroll("Single View", Color::White); break;
      case VERT_SPLIT: MatrixOS::UIUtility::TextScroll("Vertical Split", Color(0x00FFFF)); break;
      case HORIZ_SPLIT: MatrixOS::UIUtility::TextScroll("Horizontal Split", Color(0xFF00FF)); break;
    }
  });
  actionMenu.AddUIComponent(splitViewToggle, Point(0, 0));

  UIButton notepad1SelectBtn;
  notepad1SelectBtn.SetName("Note Pad 1");
  notepad1SelectBtn.SetSize(Dimension(2, 1));
  notepad1SelectBtn.SetColorFunc([&]() -> Color { return notePadConfigs[0].color.DimIfNot(0 == activeConfig); });
  notepad1SelectBtn.OnPress([&]() -> void {
    activeConfig = 0;
    octaveAbs = (int32_t)abs(notePadConfigs[activeConfig].octave);
  });
  actionMenu.AddUIComponent(notepad1SelectBtn, Point(2, 0));

  UIButton notepad2SelectBtn;
  notepad2SelectBtn.SetName("Note Pad 2");
  notepad2SelectBtn.SetSize(Dimension(2, 1));
  notepad2SelectBtn.SetColorFunc([&]() -> Color { return notePadConfigs[1].color.DimIfNot(1 == activeConfig); });
  notepad2SelectBtn.OnPress([&]() -> void {
    activeConfig = 1;
    octaveAbs = (int32_t)abs(notePadConfigs[activeConfig].octave);
  });
  actionMenu.AddUIComponent(notepad2SelectBtn, Point(4, 0));

  UIButton notepadColorBtn;
  notepadColorBtn.SetName("Note Pad Color");
  notepadColorBtn.SetColorFunc([&]() -> Color { return notePadConfigs[activeConfig].color; });
  notepadColorBtn.OnPress([&]() -> void { ColorSelector(); });
  actionMenu.AddUIComponent(notepadColorBtn, Point(7, 0));

  // Octave Control

  UI4pxNumber octaveDisplay;
  octaveDisplay.SetColorFunc([&](uint16_t digit) -> Color { return digit % 2 ? notePadConfigs[activeConfig].rootColor : notePadConfigs[activeConfig].color; });
  octaveDisplay.SetDigits(2);
  octaveDisplay.SetValuePointer(&octaveAbs);
  actionMenu.AddUIComponent(octaveDisplay, Point(0, 2));

  UIButton octaveNegSign;
  octaveNegSign.SetColor(notePadConfigs[activeConfig].rootColor);
  octaveNegSign.SetSize(Dimension(2, 1));
  octaveNegSign.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].octave < 0; });
  actionMenu.AddUIComponent(octaveNegSign, Point(2, 4));

  UIButton octavePlusBtn;
  octavePlusBtn.SetName("Octave +1");
  octavePlusBtn.SetSize(Dimension(2, 1));
  octavePlusBtn.SetColorFunc([&]() -> Color { return Color(0x80FF00).DimIfNot(notePadConfigs[activeConfig].octave < 12); });
  octavePlusBtn.OnPress([&]() -> void {
    if(notePadConfigs[activeConfig].octave < 12)
    {
      notePadConfigs[activeConfig].octave++;
      octaveAbs = (int32_t)abs(notePadConfigs[activeConfig].octave);
      configModified = true;
    }
  });
  actionMenu.AddUIComponent(octavePlusBtn, Point(4, 7));

  UIButton octaveMinusBtn;
  octaveMinusBtn.SetName("Octave -1");
  octaveMinusBtn.SetSize(Dimension(2, 1));
  octaveMinusBtn.SetColorFunc([&]() -> Color { return Color(0xFF0060).DimIfNot(notePadConfigs[activeConfig].octave > -2); });
  octaveMinusBtn.OnPress([&]() -> void {
    if(notePadConfigs[activeConfig].octave > -2)
    {
      notePadConfigs[activeConfig].octave--;
      octaveAbs = (int32_t)abs(notePadConfigs[activeConfig].octave);
      configModified = true;
    }
  });
  actionMenu.AddUIComponent(octaveMinusBtn, Point(2, 7));

  // Control Bar
  UIToggle controlBarToggle;
  controlBarToggle.SetName("Control Bar");
  controlBarToggle.SetColor(Color(0xFF8000));
  controlBarToggle.SetValuePointer(&controlBar);
  controlBarToggle.OnPress([&]() -> void {
    controlBar.Save();
    if(controlBar == false)
    {
      runtimes[0].midiPipeline.Reset();
      runtimes[1].midiPipeline.Reset();
    }
  });
  actionMenu.AddUIComponent(controlBarToggle, Point(0, 7));
  
  UIButton arpConfigBtn;
  arpConfigBtn.SetName("Arpeggiator Config");
  arpConfigBtn.SetSize(Dimension(1, 4));
  arpConfigBtn.SetColor(Color(0x80FF00));
  arpConfigBtn.OnPress([&]() -> void {ArpConfigMenu();});
  arpConfigBtn.SetEnableFunc([&]() -> bool {return controlBar;});
  actionMenu.AddUIComponent(arpConfigBtn, Point(0, 2));

  // Other Controls
  UIButton systemSettingBtn;
  systemSettingBtn.SetName("System Setting");
  systemSettingBtn.SetColor(Color::White);
  systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });
  actionMenu.AddUIComponent(systemSettingBtn, Point(7, 7));

  actionMenu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == HOLD)
      { 
        if (configModified) {
          SaveConfigs();
        }
        Exit(); 
      }
      else if (keyEvent->info.state == RELEASED)
      {
        if (configModified) {
          SaveConfigs();
          configModified = false;
        }
        PlayView();
        octaveAbs = (int32_t)abs(notePadConfigs[activeConfig].octave);
      }
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });

  actionMenu.SetGlobalLoopFunc([&]() -> void {
    Tick();
  });

  actionMenu.AllowExit(false);
  actionMenu.SetSetupFunc([&]() -> void {PlayView();});
  actionMenu.Start();

  Exit(); // This should never be reached
}

void Note::PlayView() {
  UI playView("Note Play View", notePadConfigs[activeConfig].color, false);

  Dimension padSize;
  Dimension underglowSize;

  switch (splitView)
  {
    case SINGLE_VIEW:
      padSize = Dimension(8, 8 - (controlBar ? 1 : 0));
      underglowSize = Dimension(10, 10);
      break;
    case VERT_SPLIT:
      padSize = Dimension(4, 8 - (controlBar ? 1 : 0));
      underglowSize = Dimension(5, 10);
      break;
    case HORIZ_SPLIT:
      padSize = Dimension(8, 4);
      underglowSize = Dimension(10, 5);
      break;
  }

  NotePad notePad1(padSize, &runtimes[(0 == activeConfig) ? 0 : 1]);
  playView.AddUIComponent(notePad1, Point(0, 0));

  UnderglowLight underglow1(underglowSize, notePadConfigs[(0 == activeConfig) ? 0 : 1].color);
  playView.AddUIComponent(underglow1, Point(-1, -1));

  NotePad notePad2(padSize, &runtimes[(1 == activeConfig) ? 0 : 1]);
  UnderglowLight underglow2(underglowSize, notePadConfigs[(1 == activeConfig) ? 0 : 1].color);
  
  if (splitView == VERT_SPLIT) { 
    playView.AddUIComponent(notePad2, Point(4, 0)); 
    playView.AddUIComponent(underglow2, Point(4, -1));
  }
  else if (
    splitView == HORIZ_SPLIT) { 
    playView.AddUIComponent(notePad2, Point(0, 4)); 
    playView.AddUIComponent(underglow2, Point(-1, 4));
  }
  
  NoteControlBar noteControlBar(this, &notePad1, &notePad2, &underglow1, &underglow2);
  
  if(controlBar)
  {
    playView.AddUIComponent(noteControlBar, Point(0, 8 - CTL_BAR_Y));
  }

  playView.Start();

  if(runtimes[0].noteLatch.IsEnabled())
  {
    MatrixOS::MIDI::Send(MidiPacket::ControlChange(runtimes[0].config->channel, 123, 0), MIDI_PORT_ALL); // All notes off
  }

  if(runtimes[1].noteLatch.IsEnabled())
  {
    MatrixOS::MIDI::Send(MidiPacket::ControlChange(runtimes[1].config->channel, 123, 0), MIDI_PORT_ALL); // All notes off
  }
}

void Note::ScaleSelector() {
  UI scaleSelector("Scale Selector", Color(0xFF0090), false);
  bool customScaleModified = false;
  bool configModified = false;
  uint8_t customScaleSlotSelected = 255;

  ScaleVisualizer scaleVisualizer(&notePadConfigs[activeConfig].rootKey, &notePadConfigs[activeConfig].rootOffset, &notePadConfigs[activeConfig].scale);
  scaleVisualizer.OnChange([&]() -> void { configModified = true; });
  scaleSelector.AddUIComponent(scaleVisualizer, Point(0, 0));

  UIButton offsetModeBtn;
  offsetModeBtn.SetName("Modern Diatonic");
  offsetModeBtn.SetColorFunc([&]() -> Color { return scaleVisualizer.offsetMode ? Color(0xFF0080) : Color(0x8000FF); });
  offsetModeBtn.OnPress([&]() -> void { scaleVisualizer.offsetMode = !scaleVisualizer.offsetMode; });
  scaleSelector.AddUIComponent(offsetModeBtn, Point(7, 1));

  ScaleModifier scaleModifier(&custom_scales[customScaleSlotSelected]);
  scaleModifier.SetEnableFunc([&]() -> bool { return customScaleSlotSelected != 255; });
  scaleModifier.OnChange([&](uint16_t newScale) -> void {
    customScaleModified = true;
    configModified = true;
    custom_scales[customScaleSlotSelected] = newScale;
    notePadConfigs[activeConfig].scale = newScale;
  });
  scaleSelector.AddUIComponent(scaleModifier, Point(0, 2));

  UIButton deleteScaleBtn;
  deleteScaleBtn.SetName("Delete Custom Scale");
  deleteScaleBtn.SetColor(Color(0xFF0000));
  deleteScaleBtn.SetEnableFunc([&]() -> bool { return customScaleSlotSelected != 255; });
  deleteScaleBtn.OnPress([&]() -> void {
    custom_scales[customScaleSlotSelected] = 0;
    customScaleModified = true;
    configModified = true;
    customScaleSlotSelected = 255;
    notePadConfigs[activeConfig].rootOffset = 0;
    notePadConfigs[activeConfig].scale = MINOR;
  });
  scaleSelector.AddUIComponent(deleteScaleBtn, Point(7, 3));

  UIItemSelector<uint16_t> scaleSelectorBar;
  scaleSelectorBar.SetDimension(Dimension(8, 4));
  scaleSelectorBar.SetColor(Color(0xFF0090));
  scaleSelectorBar.SetItemPointer(&notePadConfigs[activeConfig].scale);
  scaleSelectorBar.SetCount(16);
  scaleSelectorBar.SetItems(scales);
  scaleSelectorBar.SetIndividualNameFunc([&](uint16_t index) -> string { return scale_names[index]; });
  scaleSelectorBar.OnChange([&](const uint16_t& scale) -> void {
    configModified = true;
    notePadConfigs[activeConfig].rootOffset = 0;
    customScaleSlotSelected = 255;
  });
  scaleSelector.AddUIComponent(scaleSelectorBar, Point(0, 4));

  UISelector customScaleSelector;
  customScaleSelector.SetDimension(Dimension(8, 2));
  customScaleSelector.SetIndividualColorFunc([&](uint16_t index) -> Color {
    if (customScaleSlotSelected == index) {
      return Color::White;
    }
    if (custom_scales[index] == notePadConfigs[activeConfig].scale && custom_scales[index] != 0) {
      return Color(0xFFFF00);
    }
    if (custom_scales[index] != 0) {
      return Color(0xFFFF00).Dim();
    }
    return Color::White.Dim();
  });
  customScaleSelector.SetIndividualNameFunc([&](uint16_t index) -> string {
    return "Custom Scale " + std::to_string(index + 1);
  });
  customScaleSelector.OnChange([&](uint16_t index) -> void {
    if (custom_scales[index] == 0) {
      custom_scales[index] = 0b1101010110101; // Default major
      customScaleModified = true;
    }
    configModified = true;
    notePadConfigs[activeConfig].scale = custom_scales[index];
    customScaleSlotSelected = index;
    scaleModifier.ChangeScalePtr(&custom_scales[index]);
  });
  scaleSelector.AddUIComponent(customScaleSelector, Point(0, 6));

  scaleSelector.Start();

  if (customScaleModified) {
    MatrixOS::NVS::SetVariable(CUSTOM_SCALES_HASH, custom_scales, sizeof(custom_scales));
  }

  if (configModified) {
    SaveConfigs();
  }
}

void Note::ColorSelector() {
  UI colorSelector("Color Selector", notePadConfigs[activeConfig].color, false);
  bool configModified = false;
  uint8_t page = 0;  // 0 = Preset, 1 = Customize

  // Create NotePadRuntime structure for color selector
  NotePadRuntime colorSelectorData;
  colorSelectorData.config = &notePadConfigs[activeConfig];

  NotePad notePad(Dimension(8, 4), &colorSelectorData);
  colorSelector.AddUIComponent(notePad, Point(0, 0));

  UIButton presetsBtn;
  presetsBtn.SetName("Preset");
  presetsBtn.SetColorFunc([&]() -> Color {
    return Color::White.DimIfNot(page == 0);
  });
  presetsBtn.OnPress([&]() -> void { page = 0; });
  colorSelector.AddUIComponent(presetsBtn, Point(0, 5));

  UIButton customizeBtn;
  customizeBtn.SetName("Customize");
  customizeBtn.SetColorFunc([&]() -> Color {
    return Color::White.DimIfNot(page == 1);
  });
  customizeBtn.OnPress([&]() -> void { page = 1; });
  colorSelector.AddUIComponent(customizeBtn, Point(0, 6));

  UIButton preset1Btn;
  preset1Btn.SetName("Preset1");
  preset1Btn.SetColorFunc([&]() -> Color {
    bool selected =
      (notePadConfigs[activeConfig].colorMode == ROOT_N_SCALE) &&
      (notePadConfigs[activeConfig].rootColor == colorPresets[0][0]) &&
      (notePadConfigs[activeConfig].color == colorPresets[0][1]);
    return Color(colorPresets[0][1]).DimIfNot(selected);
  });
  preset1Btn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].colorMode = ROOT_N_SCALE;
    notePadConfigs[activeConfig].rootColor = colorPresets[0][0];
    notePadConfigs[activeConfig].color = colorPresets[0][1];
  });
  preset1Btn.SetEnableFunc([&]() -> bool { return page == 0; });
  colorSelector.AddUIComponent(preset1Btn, Point(2, 5));

  UIButton preset2Btn;
  preset2Btn.SetName("Preset2");
  preset2Btn.SetColorFunc([&]() -> Color {
    bool selected =
      (notePadConfigs[activeConfig].colorMode == ROOT_N_SCALE) &&
      (notePadConfigs[activeConfig].rootColor == colorPresets[1][0]) &&
      (notePadConfigs[activeConfig].color == colorPresets[1][1]);
    return Color(colorPresets[1][1]).DimIfNot(selected);
  });
  preset2Btn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].colorMode = ROOT_N_SCALE;
    notePadConfigs[activeConfig].rootColor = colorPresets[1][0];
    notePadConfigs[activeConfig].color = colorPresets[1][1];
  });
  preset2Btn.SetEnableFunc([&]() -> bool { return page == 0; });
  colorSelector.AddUIComponent(preset2Btn, Point(3, 5));

  UIButton preset3Btn;
  preset3Btn.SetName("Preset3");
  preset3Btn.SetColorFunc([&]() -> Color {
    bool selected =
      (notePadConfigs[activeConfig].colorMode == ROOT_N_SCALE) &&
      (notePadConfigs[activeConfig].rootColor == colorPresets[2][0]) &&
      (notePadConfigs[activeConfig].color == colorPresets[2][1]);
    return Color(colorPresets[2][1]).DimIfNot(selected);
  });
  preset3Btn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].colorMode = ROOT_N_SCALE;
    notePadConfigs[activeConfig].rootColor = colorPresets[2][0];
    notePadConfigs[activeConfig].color = colorPresets[2][1];
  });
  preset3Btn.SetEnableFunc([&]() -> bool { return page == 0; });
  colorSelector.AddUIComponent(preset3Btn, Point(4, 5));

  UIButton preset4Btn;
  preset4Btn.SetName("Preset4");
  preset4Btn.SetColorFunc([&]() -> Color {
    bool selected =
      (notePadConfigs[activeConfig].colorMode == ROOT_N_SCALE) &&
      (notePadConfigs[activeConfig].rootColor == colorPresets[3][0]) &&
      (notePadConfigs[activeConfig].color == colorPresets[3][1]);
    return Color(colorPresets[3][1]).DimIfNot(selected);
  });
  preset4Btn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].colorMode = ROOT_N_SCALE;
    notePadConfigs[activeConfig].rootColor = colorPresets[3][0];
    notePadConfigs[activeConfig].color = colorPresets[3][1];
  });
  preset4Btn.SetEnableFunc([&]() -> bool { return page == 0; });
  colorSelector.AddUIComponent(preset4Btn, Point(5, 5));

  UIButton preset5Btn;
  preset5Btn.SetName("Preset5");
  preset5Btn.SetColorFunc([&]() -> Color {
    bool selected =
      (notePadConfigs[activeConfig].colorMode == ROOT_N_SCALE) &&
      (notePadConfigs[activeConfig].rootColor == colorPresets[4][0]) &&
      (notePadConfigs[activeConfig].color == colorPresets[4][1]);
    return Color(colorPresets[4][1]).DimIfNot(selected);
  });
  preset5Btn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].colorMode = ROOT_N_SCALE;
    notePadConfigs[activeConfig].rootColor = colorPresets[4][0];
    notePadConfigs[activeConfig].color = colorPresets[4][1];
  });
  preset5Btn.SetEnableFunc([&]() -> bool { return page == 0; });
  colorSelector.AddUIComponent(preset5Btn, Point(2, 6));

  UIButton preset6Btn;
  preset6Btn.SetName("Preset6");
  preset6Btn.SetColorFunc([&]() -> Color {
    bool selected =
      (notePadConfigs[activeConfig].colorMode == ROOT_N_SCALE) &&
      (notePadConfigs[activeConfig].rootColor == colorPresets[5][0]) &&
      (notePadConfigs[activeConfig].color == colorPresets[5][1]);
    return Color(colorPresets[5][1]).DimIfNot(selected);
  });
  preset6Btn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].colorMode = ROOT_N_SCALE;
    notePadConfigs[activeConfig].rootColor = colorPresets[5][0];
    notePadConfigs[activeConfig].color = colorPresets[5][1];
  });
  preset6Btn.SetEnableFunc([&]() -> bool { return page == 0; });
  colorSelector.AddUIComponent(preset6Btn, Point(3, 6));

  UIButton rainbowColorBtn;
  rainbowColorBtn.SetName("Rainbow Color");
  rainbowColorBtn.SetColorFunc([&]() -> Color {
    bool selected = (notePadConfigs[activeConfig].colorMode == COLOR_PER_KEY_RAINBOW);
    return  ColorEffects::Rainbow(2000).DimIfNot(selected);
  });
  rainbowColorBtn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].colorMode = COLOR_PER_KEY_RAINBOW;
  });
  rainbowColorBtn.SetEnableFunc([&]() -> bool { return page == 0; });
  colorSelector.AddUIComponent(rainbowColorBtn, Point(4, 6));

  UIButton polyColorBtn;
  polyColorBtn.SetName("Poly Color");
  polyColorBtn.SetColorFunc([&]() -> Color {
    bool selected = (notePadConfigs[activeConfig].colorMode == COLOR_PER_KEY_POLY);
    uint32_t index = MatrixOS::SYS::Millis() % (12 * 500) / 500;
    Color color = polyNoteColor[index];
    return color.DimIfNot(selected);
  });
  polyColorBtn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].colorMode = COLOR_PER_KEY_POLY;
  });
  polyColorBtn.SetEnableFunc([&]() -> bool { return page == 0; });
  colorSelector.AddUIComponent(polyColorBtn, Point(5, 6));


  // Customize
  UIButton rootColorSelectorBtn;
  rootColorSelectorBtn.SetName("Root Key Color");
  rootColorSelectorBtn.SetColorFunc([&]() -> Color { return notePadConfigs[activeConfig].rootColor; });
  rootColorSelectorBtn.SetSize(Dimension(2, 2));
  rootColorSelectorBtn.OnPress([&]() -> void {
    MatrixOS::UIUtility::ColorPicker(notePadConfigs[activeConfig].rootColor);
    configModified = true;
  });
  rootColorSelectorBtn.SetEnableFunc([&]() -> bool { return page == 1; });
  colorSelector.AddUIComponent(rootColorSelectorBtn, Point(2, 5));

  UIButton notePadColorSelectorBtn;
  notePadColorSelectorBtn.SetName("Note Pad Color");
  notePadColorSelectorBtn.SetColorFunc([&]() -> Color { return notePadConfigs[activeConfig].color; });
  notePadColorSelectorBtn.SetSize(Dimension(2, 2));
  notePadColorSelectorBtn.OnPress([&]() -> void {
    MatrixOS::UIUtility::ColorPicker(notePadConfigs[activeConfig].color);
    configModified = true;
  });
  notePadColorSelectorBtn.SetEnableFunc([&]() -> bool { return page == 1; });
  colorSelector.AddUIComponent(notePadColorSelectorBtn, Point(4, 5));

  UIButton whiteOutOfScaleToggle;
  whiteOutOfScaleToggle.SetName("White Out of Scale");
  whiteOutOfScaleToggle.SetColorFunc([&]() -> Color {
    return notePadConfigs[activeConfig].useWhiteAsOutOfScale ? Color(0x202020) : notePadConfigs[activeConfig].color.Dim(32);
  });
  whiteOutOfScaleToggle.SetSize(Dimension(1, 2));
  whiteOutOfScaleToggle.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].useWhiteAsOutOfScale = !notePadConfigs[activeConfig].useWhiteAsOutOfScale;
  });
  colorSelector.AddUIComponent(whiteOutOfScaleToggle, Point(7, 5));

  colorSelector.Start();

  if (configModified) {
    SaveConfigs();
  }
}

void Note::LayoutSelector() {
  const Color color = Color(0xFFFF00);
  UI layoutSelector("Layout Selector", color, false);
  bool configModified = false;

  int32_t x_offset = notePadConfigs[activeConfig].x_offset;
  int32_t y_offset = notePadConfigs[activeConfig].y_offset;

  UIButton octaveModeBtn;
  octaveModeBtn.SetName("Octave Mode");
  octaveModeBtn.SetColorFunc([&]() -> Color { Color c = color; return c.DimIfNot(notePadConfigs[activeConfig].mode == OCTAVE_LAYOUT); });
  octaveModeBtn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].mode = OCTAVE_LAYOUT;
  });
  layoutSelector.AddUIComponent(octaveModeBtn, Point(2, 0));

  UIButton offsetModeBtn;
  offsetModeBtn.SetName("Offset Mode");
  offsetModeBtn.SetColorFunc([&]() -> Color { Color c = color; return c.DimIfNot(notePadConfigs[activeConfig].mode == OFFSET_LAYOUT); });
  offsetModeBtn.OnPress([&]() -> void {
    if(notePadConfigs[activeConfig].mode != OFFSET_LAYOUT)
    {
      configModified = true;
      notePadConfigs[activeConfig].mode = OFFSET_LAYOUT;
      x_offset = 1;
      y_offset = 3;
      notePadConfigs[activeConfig].x_offset = x_offset;
      notePadConfigs[activeConfig].y_offset = y_offset;
    }
  });
  layoutSelector.AddUIComponent(offsetModeBtn, Point(3, 0));

  UIButton chromaticModeBtn;
  chromaticModeBtn.SetName("Chromatic Mode");
  chromaticModeBtn.SetColorFunc([&]() -> Color { Color c = color; return c.DimIfNot(notePadConfigs[activeConfig].mode == CHROMATIC_LAYOUT); });
  chromaticModeBtn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].mode = CHROMATIC_LAYOUT;
  });
  layoutSelector.AddUIComponent(chromaticModeBtn, Point(4, 0));

  UIButton pianoModeBtn;
  pianoModeBtn.SetName("Piano Keyboard");
  pianoModeBtn.SetColorFunc([&]() -> Color { Color c = color; return c.DimIfNot(notePadConfigs[activeConfig].mode == PIANO_LAYOUT); });
  pianoModeBtn.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].mode = PIANO_LAYOUT;
  });
  layoutSelector.AddUIComponent(pianoModeBtn, Point(5, 0));

  // Octave mode
  UITimedDisplay octTextDisplay(UINT32_MAX);
  octTextDisplay.SetDimension(Dimension(8, 4));
  octTextDisplay.SetRenderFunc([&](Point origin) -> void {
    // O
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // C
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // T
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
  });
  octTextDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OCTAVE_LAYOUT; });
  layoutSelector.AddUIComponent(octTextDisplay, Point(0, 2));


  // Offset Mode
  const Color xColor = Color(0x00FFFF);
  const Color yColor = Color(0xFF00FF);

  UI4pxNumber yOffsetDisplay;
  yOffsetDisplay.SetColor(yColor);
  yOffsetDisplay.SetDigits(1);
  yOffsetDisplay.SetValuePointer((int32_t*)&y_offset);
  yOffsetDisplay.SetAlternativeColor(yColor);
  yOffsetDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(yOffsetDisplay, Point(2, 2));

  UI4pxNumber xOffsetDisplay;
  xOffsetDisplay.SetColor(xColor);
  xOffsetDisplay.SetDigits(1);
  xOffsetDisplay.SetValuePointer((int32_t*)&x_offset);
  xOffsetDisplay.SetAlternativeColor(xColor);
  xOffsetDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(xOffsetDisplay, Point(5, 2));

  UITimedDisplay ofsTextDisplay(500);
  ofsTextDisplay.SetDimension(Dimension(6, 4));
  ofsTextDisplay.SetRenderFunc([&](Point origin) -> void {
    // Y
    MatrixOS::LED::SetColor(origin + Point(0, 0), yColor);
    MatrixOS::LED::SetColor(origin + Point(0, 1), yColor);
    MatrixOS::LED::SetColor(origin + Point(1, 2), yColor);
    MatrixOS::LED::SetColor(origin + Point(1, 3), yColor);
    MatrixOS::LED::SetColor(origin + Point(2, 0), yColor);
    MatrixOS::LED::SetColor(origin + Point(2, 1), yColor);

    // X
    MatrixOS::LED::SetColor(origin + Point(3, 0), xColor);
    MatrixOS::LED::SetColor(origin + Point(3, 3), xColor);
    MatrixOS::LED::SetColor(origin + Point(4, 1), xColor);
    MatrixOS::LED::SetColor(origin + Point(4, 2), xColor);
    MatrixOS::LED::SetColor(origin + Point(5, 0), xColor);
    MatrixOS::LED::SetColor(origin + Point(5, 3), xColor);
  });
  ofsTextDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(ofsTextDisplay, Point(2, 2));

  UISelector yOffsetInput;
  yOffsetInput.SetDimension(Dimension(1, 8));
  yOffsetInput.SetName("Y Offset");
  yOffsetInput.SetColor(yColor);
  yOffsetInput.SetValuePointer((uint16_t*)&y_offset);
  yOffsetInput.SetLitMode(UISelectorLitMode::LIT_LESS_EQUAL_THAN);
  yOffsetInput.SetDirection(UISelectorDirection::UP_THEN_RIGHT);
  yOffsetInput.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  yOffsetInput.OnChange([&](uint16_t val) -> void {
    configModified = true;
    notePadConfigs[activeConfig].y_offset = val;
    ofsTextDisplay.Disable();
  });
  layoutSelector.AddUIComponent(yOffsetInput, Point(0, 0));

  UISelector xOffsetInput;
  xOffsetInput.SetDimension(Dimension(8, 1));
  xOffsetInput.SetName("X Offset");
  xOffsetInput.SetColor(xColor);
  xOffsetInput.SetValuePointer((uint16_t*)&x_offset);
  xOffsetInput.SetLitMode(UISelectorLitMode::LIT_LESS_EQUAL_THAN);
  xOffsetInput.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  xOffsetInput.OnChange([&](uint16_t val) -> void {
    configModified = true;
    notePadConfigs[activeConfig].x_offset = val;
    ofsTextDisplay.Disable();
  });
  layoutSelector.AddUIComponent(xOffsetInput, Point(0, 7));

  // Chromatic
  UITimedDisplay chmTextDisplay(UINT32_MAX);
  chmTextDisplay.SetDimension(Dimension(8, 4));
  chmTextDisplay.SetRenderFunc([&](Point origin) -> void {
    // C
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);

    // H
    MatrixOS::LED::SetColor(origin + Point(2, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // M
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
  });
  chmTextDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == CHROMATIC_LAYOUT; });
  layoutSelector.AddUIComponent(chmTextDisplay, Point(0, 2));

  // Piano
  UITimedDisplay pioTextDisplay(UINT32_MAX);
  pioTextDisplay.SetDimension(Dimension(8, 4));
  pioTextDisplay.SetRenderFunc([&](Point origin) -> void {
    // P
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);

    // I
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);


    // O
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
  });
  pioTextDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == PIANO_LAYOUT; });
  layoutSelector.AddUIComponent(pioTextDisplay, Point(0, 2));

  // Show Enforce Scale Toggle (only for Octave, Offset, and Chromatic modes)
  UIButton enforceScaleToggle;
  enforceScaleToggle.SetName("Enforce Scale");
  enforceScaleToggle.SetColorFunc([&]() -> Color { return Color::White.DimIfNot(notePadConfigs[activeConfig].enforceScale); });
  enforceScaleToggle.OnPress([&]() -> void {
    configModified = true;
    notePadConfigs[activeConfig].enforceScale = !notePadConfigs[activeConfig].enforceScale;
  });
  enforceScaleToggle.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OCTAVE_LAYOUT || notePadConfigs[activeConfig].mode == OFFSET_LAYOUT || notePadConfigs[activeConfig].mode == CHROMATIC_LAYOUT;; });
  layoutSelector.AddUIComponent(enforceScaleToggle, Point(0, 7));

  layoutSelector.Start();

  if (configModified) {
    SaveConfigs();
  }
}

void Note::ChannelSelector() {
  Color color = Color(0x60FF00);
  UI channelSelector("Channel Selector", color, false);
  bool configModified = false;

  int32_t offsettedChannel = notePadConfigs[activeConfig].channel + 1;
  UI4pxNumber numDisplay;
  numDisplay.SetColor(color);
  numDisplay.SetDigits(2);
  numDisplay.SetValuePointer(&offsettedChannel);
  numDisplay.SetAlternativeColor(Color::White);
  numDisplay.SetSpacing(1);
  channelSelector.AddUIComponent(numDisplay, Point(1, 0));

  UISelector channelInput;
  channelInput.SetDimension(Dimension(8, 2));
  channelInput.SetName("Channel");
  channelInput.SetColor(color);
  channelInput.SetCount(16);
  channelInput.SetValuePointer((uint16_t*)&notePadConfigs[activeConfig].channel);
  channelInput.OnChange([&](uint16_t val) -> void {
    configModified = true;
    offsettedChannel = val + 1;
  });

  channelSelector.AddUIComponent(channelInput, Point(0, 6));

  channelSelector.SetPostRenderFunc([&]() -> void {
    // C
    MatrixOS::LED::SetColor(Point(0, 0), color);
    MatrixOS::LED::SetColor(Point(0, 1), color);
    MatrixOS::LED::SetColor(Point(0, 2), color);
    MatrixOS::LED::SetColor(Point(0, 3), color);
    MatrixOS::LED::SetColor(Point(1, 0), color);
    MatrixOS::LED::SetColor(Point(1, 3), color);

    if(notePadConfigs[activeConfig].channel < 9)
    {
      //h
      MatrixOS::LED::SetColor(Point(2, 0), Color::White);
      MatrixOS::LED::SetColor(Point(2, 1), Color::White);
      MatrixOS::LED::SetColor(Point(2, 2), Color::White);
      MatrixOS::LED::SetColor(Point(2, 3), Color::White);
      MatrixOS::LED::SetColor(Point(3, 1), Color::White);
      MatrixOS::LED::SetColor(Point(4, 0), Color::White);
      MatrixOS::LED::SetColor(Point(4, 1), Color::White);
      MatrixOS::LED::SetColor(Point(4, 2), Color::White);
      MatrixOS::LED::SetColor(Point(4, 3), Color::White);
    }
  });

  channelSelector.Start();

  if (configModified) {
    SaveConfigs();
  }
}

void Note::ArpConfigMenu() {
  UI arpConfigMenu("Arpeggiator Config", Color(0x80FF00));
  bool configModified = false;
  uint64_t menuOpenTime = MatrixOS::SYS::Millis();

  enum ArpConfigType:uint16_t
  {
    ARP_BPM,
    ARP_SWING,
    ARP_GATE,
    ARP_DIRECTION,
    ARP_STEP,
    ARP_STEP_OFFSET,
    ARP_REPEAT,
    ARP_RHYTHM
  };

  Color arpConfigColor[8]
  {
    Color(0xFF0000), // Red - BPM
    Color(0xFF8000), // Orange - Swing
    Color(0xFFFF00), // Yellow - Gate
    Color(0x00FF00), // Green - Direction
    Color(0x00FFFF), // Cyan - Step
    Color(0x4000FF), // Blue - Step Offset
    Color(0xFF00FF), // Magenta - Repeat
    Color(0xFF0040) // Hot Pink - Rhythm
  };

  string arpConfigName[8]
  {
    "BPM",
    "Swing",
    "Gate",
    "Direction",
    "Step",
    "Step Offset",
    "Repeat",
    "Rhythm",
  };

  // Shared arrays for all number modifiers
  const int32_t coarseModifier[8] = {-25, -10, -5, -1, 1, 5, 10, 25};
  const int32_t fineModifier[8] = {-10, -5, -2, -1, 1, 2, 5, 10};
  const uint8_t modifierGradient[8] = {255, 127, 64, 32, 32, 64, 127, 255};

  UISelector arpConfigSelector;
  arpConfigSelector.SetCount(8);
  arpConfigSelector.SetDimension(Dimension(8, 1));
  arpConfigSelector.SetIndividualColorFunc([&](uint16_t index) -> Color { return arpConfigColor[index]; });
  arpConfigSelector.SetIndividualNameFunc([&](uint16_t index) -> string { return arpConfigName[index]; });
  arpConfigSelector.SetValuePointer((uint16_t*)&arpMenuPage);
  arpConfigSelector.OnChange([&](uint16_t val) -> void {
    if(arpMenuPage != val) {
      arpMenuPage = val;
      menuOpenTime = MatrixOS::SYS::Millis();
    }
  });
  arpConfigMenu.AddUIComponent(arpConfigSelector, Point(0, 0));

  // BPM selector
  UITimedDisplay bpmTextDisplay(500);
  bpmTextDisplay.SetDimension(Dimension(8, 4));
  bpmTextDisplay.SetRenderFunc([&](Point origin) -> void {
    Color color = arpConfigColor[ARP_BPM];

    // B
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // P
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);

    // M
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
  });
  bpmTextDisplay.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_BPM; });
  arpConfigMenu.AddUIComponent(bpmTextDisplay, Point(0, 2));

  int32_t bpmValue = bpm;

  UI4pxNumber bpmDisplay;
  bpmDisplay.SetColor(arpConfigColor[ARP_BPM]);
  bpmDisplay.SetDigits(3);
  bpmDisplay.SetValuePointer(&bpmValue);
  bpmDisplay.SetAlternativeColor(Color::White);
  bpmDisplay.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_BPM) &&  !bpmTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(bpmDisplay, Point(-1, 2));

  UINumberModifier bpmNumberModifier;
  bpmNumberModifier.SetColor(arpConfigColor[ARP_BPM]);
  bpmNumberModifier.SetLength(8);
  bpmNumberModifier.SetValuePointer(&bpmValue);
  bpmNumberModifier.SetModifiers(coarseModifier);
  bpmNumberModifier.SetControlGradient(modifierGradient);
  bpmNumberModifier.SetLowerLimit(20);
  bpmNumberModifier.SetUpperLimit(299);
  bpmNumberModifier.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_BPM; });
  bpmNumberModifier.OnChange([&](int32_t val) -> void {
    bpmValue = val;
    bpm = (uint16_t)val;
    midiClock.SetBPM((uint16_t)val);
    bpmTextDisplay.Disable();
  });
  arpConfigMenu.AddUIComponent(bpmNumberModifier, Point(0, 7));

  UIButton clockOutBtn;
  clockOutBtn.SetSize(Dimension(1, 1));
  clockOutBtn.SetColorFunc([&]() -> Color {
    if(clockMode == CLOCK_EXTERNAL)
    {
      return Color(0xFF00FF);
    }
    else if(clockMode == CLOCK_INTERNAL_CLOCKOUT)
    {
      return Color(0x80FF00);
    }
    else if(clockMode == CLOCK_INTERNAL)
    {
      return Color(0x80FF00).Dim();
    }
    return Color(0xFF0000);
  });
  clockOutBtn.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_BPM; });
  clockOutBtn.OnPress([&]() -> void {;
    if (clockMode == CLOCK_INTERNAL) {
      clockMode = CLOCK_INTERNAL_CLOCKOUT;
    } else {
      clockMode = CLOCK_INTERNAL;
    }
  });
  arpConfigMenu.AddUIComponent(clockOutBtn, Point(0, 6));

  // Swing selector
  UITimedDisplay swingTextDisplay(500);
  swingTextDisplay.SetDimension(Dimension(8, 4));
  swingTextDisplay.SetRenderFunc([&](Point origin) -> void {
    Color color = arpConfigColor[ARP_SWING];

    // S
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);

    // W
    MatrixOS::LED::SetColor(origin + Point(2, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // G
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
  });
  swingTextDisplay.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_SWING; });
  arpConfigMenu.AddUIComponent(swingTextDisplay, Point(0, 2));

  int32_t swingValue = (int32_t)notePadConfigs[activeConfig].arpConfig.swing;

  UI4pxNumber swingDisplay;
  swingDisplay.SetColor(arpConfigColor[ARP_SWING]);
  swingDisplay.SetDigits(3);
  swingDisplay.SetValuePointer(&swingValue);
  swingDisplay.SetAlternativeColor(Color::White);
  swingDisplay.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_SWING) && !swingTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(swingDisplay, Point(-1, 2));

  UINumberModifier swingNumberModifier;
  swingNumberModifier.SetColor(arpConfigColor[ARP_SWING]);
  swingNumberModifier.SetLength(8);
  swingNumberModifier.SetValuePointer(&swingValue);
  swingNumberModifier.SetModifiers(fineModifier);
  swingNumberModifier.SetControlGradient(modifierGradient);
  swingNumberModifier.SetLowerLimit(20);
  swingNumberModifier.SetUpperLimit(80);
  swingNumberModifier.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_SWING; });
  swingNumberModifier.OnChange([&](int32_t val) -> void {
    swingTextDisplay.Disable();
    configModified = true;
    notePadConfigs[activeConfig].arpConfig.swing = val;
    runtimes[activeConfig].arpeggiator.UpdateConfig();
  });
  arpConfigMenu.AddUIComponent(swingNumberModifier, Point(0, 7));

  // Gate selector
  UITimedDisplay gateTextDisplay(500);
  gateTextDisplay.SetDimension(Dimension(8, 4));
  gateTextDisplay.SetRenderFunc([&](Point origin) -> void {
    Color color = arpConfigColor[ARP_GATE];

    // G
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // A
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 3), Color::White);

    // T
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(8, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
  });
  gateTextDisplay.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_GATE; });
  arpConfigMenu.AddUIComponent(gateTextDisplay, Point(0, 2));

  int32_t gateValue = notePadConfigs[activeConfig].arpConfig.gateTime;

  UI4pxNumber gateDisplay;
  gateDisplay.SetColor(arpConfigColor[ARP_GATE]);
  gateDisplay.SetDigits(3);
  gateDisplay.SetValuePointer(&gateValue);
  gateDisplay.SetAlternativeColor(Color::White);
  gateDisplay.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_GATE) && !gateTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(gateDisplay, Point(-1, 2));

  InfDisplay gateInfDisplay(arpConfigColor[ARP_GATE]);
  gateInfDisplay.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_GATE) && notePadConfigs[activeConfig].arpConfig.gateTime == 0 && !gateTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(gateInfDisplay, Point(0, 2));

  UINumberModifier gateNumberModifier;
  gateNumberModifier.SetColor(arpConfigColor[ARP_GATE]);
  gateNumberModifier.SetLength(8);
  gateNumberModifier.SetValuePointer(&gateValue);
  gateNumberModifier.SetModifiers(fineModifier);
  gateNumberModifier.SetControlGradient(modifierGradient);
  gateNumberModifier.SetLowerLimit(0);
  gateNumberModifier.SetUpperLimit(200);
  gateNumberModifier.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_GATE; });
  gateNumberModifier.OnChange([&](int32_t val) -> void {
    gateValue = val;
    configModified = true;
    notePadConfigs[activeConfig].arpConfig.gateTime = (uint8_t)val;
    gateTextDisplay.Disable();
    runtimes[activeConfig].arpeggiator.UpdateConfig();
  });
  arpConfigMenu.AddUIComponent(gateNumberModifier, Point(0, 7));

  // Direction Selector
  UITimedDisplay directionTextDisplay(500);
  directionTextDisplay.SetDimension(Dimension(8, 4));
  directionTextDisplay.SetRenderFunc([&](Point origin) -> void {
    Color color = arpConfigColor[ARP_DIRECTION];

    // D
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);
    MatrixOS::LED::SetColor(origin + Point(3, 1), color);
    MatrixOS::LED::SetColor(origin + Point(3, 2), color);

    // I
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // R
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(5, 1), color);
    MatrixOS::LED::SetColor(origin + Point(5, 2), color);
    MatrixOS::LED::SetColor(origin + Point(5, 3), color);
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
  });
  directionTextDisplay.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_DIRECTION; });
  arpConfigMenu.AddUIComponent(directionTextDisplay, Point(0, 2));

  ArpDirVisualizer arpDirVisualizer(&notePadConfigs[activeConfig].arpConfig.direction, arpConfigColor[ARP_DIRECTION]);
  arpDirVisualizer.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_DIRECTION && !directionTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(arpDirVisualizer, Point(0, 2));

  UISelector directionSelector;
  directionSelector.SetDimension(Dimension(8, 4));
  directionSelector.SetColor(arpConfigColor[ARP_DIRECTION]);
  directionSelector.SetValueFunc([&]() -> uint16_t { return (uint16_t)notePadConfigs[activeConfig].arpConfig.direction; });
  directionSelector.OnChange([&](uint16_t value) -> void {
    configModified = true;
    notePadConfigs[activeConfig].arpConfig.direction = (ArpDirection)value;
    directionTextDisplay.Disable();
    runtimes[activeConfig].arpeggiator.UpdateConfig();
  });
  directionSelector.SetCount(16);
  directionSelector.SetIndividualNameFunc([&](uint16_t index) -> string { return arpDirectionNames[index]; });
  directionSelector.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_DIRECTION; });
  arpConfigMenu.AddUIComponent(directionSelector, Point(0, 6));

  // Step selector
  UITimedDisplay stepTextDisplay(500);
  stepTextDisplay.SetDimension(Dimension(8, 4));
  stepTextDisplay.SetRenderFunc([&](Point origin) -> void {
    Color color = arpConfigColor[ARP_STEP];

    // S
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // T
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // P
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
  });
  stepTextDisplay.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_STEP; });
  arpConfigMenu.AddUIComponent(stepTextDisplay, Point(0, 2));

  int32_t stepValue = notePadConfigs[activeConfig].arpConfig.step;

  UI4pxNumber stepDisplay;
  stepDisplay.SetColor(arpConfigColor[ARP_STEP]);
  stepDisplay.SetDigits(3);
  stepDisplay.SetValuePointer(&stepValue);
  stepDisplay.SetAlternativeColor(Color::White);
  stepDisplay.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_STEP) && !stepTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(stepDisplay, Point(-1, 2));

  // Custom modifier for step (1-16 range)

  UINumberModifier stepNumberModifier;
  stepNumberModifier.SetColor(arpConfigColor[ARP_STEP]);
  stepNumberModifier.SetLength(8);
  stepNumberModifier.SetValuePointer(&stepValue);
  stepNumberModifier.SetModifiers(fineModifier);
  stepNumberModifier.SetControlGradient(modifierGradient);
  stepNumberModifier.SetLowerLimit(1);
  stepNumberModifier.SetUpperLimit(16);
  stepNumberModifier.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_STEP; });
  stepNumberModifier.OnChange([&](int32_t val) -> void {
    stepValue = val;
    configModified = true;
    notePadConfigs[activeConfig].arpConfig.step = (uint8_t)val;
    stepTextDisplay.Disable();
    runtimes[activeConfig].arpeggiator.UpdateConfig();
  });
  arpConfigMenu.AddUIComponent(stepNumberModifier, Point(0, 7));

  // Step Offset selector (with minus sign support)
  UITimedDisplay offsetTextDisplay(500);
  offsetTextDisplay.SetDimension(Dimension(8, 4));
  offsetTextDisplay.SetRenderFunc([&](Point origin) -> void {
    Color color = arpConfigColor[ARP_STEP_OFFSET];

    // O
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // F
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 0), Color::White);

    // S
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(7, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 3), color);
  });
  offsetTextDisplay.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_STEP_OFFSET; });
  arpConfigMenu.AddUIComponent(offsetTextDisplay, Point(0, 2));

  int32_t stepOffsetValue = notePadConfigs[activeConfig].arpConfig.stepOffset;
  int32_t stepOffsetDisplayValue = abs(stepOffsetValue);

  UIButton stepOffsetNegSign;
  stepOffsetNegSign.SetColor(arpConfigColor[ARP_STEP_OFFSET]);
  stepOffsetNegSign.SetSize(Dimension(2, 1));
  stepOffsetNegSign.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_STEP_OFFSET) && !offsetTextDisplay.IsEnabled() && stepOffsetValue < 0; });
  arpConfigMenu.AddUIComponent(stepOffsetNegSign, Point(0, 4));

  UI4pxNumber stepOffsetDisplay;
  stepOffsetDisplay.SetColor(arpConfigColor[ARP_STEP_OFFSET]);
  stepOffsetDisplay.SetDigits(2);
  stepOffsetDisplay.SetValuePointer(&stepOffsetDisplayValue);
  stepOffsetDisplay.SetAlternativeColor(Color::White);
  stepOffsetDisplay.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_STEP_OFFSET) && !offsetTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(stepOffsetDisplay, Point(2, 2));

  // Custom modifier for step offset (-24 to 24 semitones)
  UINumberModifier stepOffsetNumberModifier;
  stepOffsetNumberModifier.SetColor(arpConfigColor[ARP_STEP_OFFSET]);
  stepOffsetNumberModifier.SetLength(8);
  stepOffsetNumberModifier.SetValuePointer(&stepOffsetValue);
  stepOffsetNumberModifier.SetModifiers(fineModifier);
  stepOffsetNumberModifier.SetControlGradient(modifierGradient);
  stepOffsetNumberModifier.SetLowerLimit(-48);
  stepOffsetNumberModifier.SetUpperLimit(48);
  stepOffsetNumberModifier.OnChange([&](int32_t value) -> void {
    stepOffsetValue = value;
    configModified = true;
    notePadConfigs[activeConfig].arpConfig.stepOffset = (int8_t)value;
    stepOffsetDisplayValue = abs(value);
    offsetTextDisplay.Disable();
    runtimes[activeConfig].arpeggiator.UpdateConfig();
  });
  stepOffsetNumberModifier.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_STEP_OFFSET; });
  arpConfigMenu.AddUIComponent(stepOffsetNumberModifier, Point(0, 7));

  // Repeat selector
  UITimedDisplay repeatTextDisplay(500);
  repeatTextDisplay.SetDimension(Dimension(8, 4));
  repeatTextDisplay.SetRenderFunc([&](Point origin) -> void {
    Color color = arpConfigColor[ARP_REPEAT];

    // R
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 0), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // E
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(5, 3), Color::White);

    // P
    MatrixOS::LED::SetColor(origin + Point(6, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
    MatrixOS::LED::SetColor(origin + Point(7, 1), color);
  });
  repeatTextDisplay.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_REPEAT; });
  arpConfigMenu.AddUIComponent(repeatTextDisplay, Point(0, 2));

  int32_t repeatValue = notePadConfigs[activeConfig].arpConfig.repeat;

  UI4pxNumber repeatDisplay;
  repeatDisplay.SetColor(arpConfigColor[ARP_REPEAT]);
  repeatDisplay.SetDigits(3);
  repeatDisplay.SetValuePointer(&repeatValue);
  repeatDisplay.SetAlternativeColor(Color::White);
  repeatDisplay.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_REPEAT) && !repeatTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(repeatDisplay, Point(-1, 2));
  
  InfDisplay repeatInfDisplay(arpConfigColor[ARP_REPEAT]);
  repeatInfDisplay.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_REPEAT) && notePadConfigs[activeConfig].arpConfig.repeat == 0 && !repeatTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(repeatInfDisplay, Point(0, 2));

  UINumberModifier repeatNumberModifier;
  repeatNumberModifier.SetColor(arpConfigColor[ARP_REPEAT]);
  repeatNumberModifier.SetLength(8);
  repeatNumberModifier.SetValuePointer(&repeatValue);
  repeatNumberModifier.SetModifiers(fineModifier);
  repeatNumberModifier.SetControlGradient(modifierGradient);
  repeatNumberModifier.SetLowerLimit(0);
  repeatNumberModifier.SetUpperLimit(100);
  repeatNumberModifier.OnChange([&](int32_t value) -> void {
    repeatValue = value;
    configModified = true;
    notePadConfigs[activeConfig].arpConfig.repeat = (uint8_t)value;
    repeatTextDisplay.Disable();
    runtimes[activeConfig].arpeggiator.UpdateConfig();
  });
  repeatNumberModifier.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_REPEAT; });
  arpConfigMenu.AddUIComponent(repeatNumberModifier, Point(0, 7));

  // Rhythm Page
  UITimedDisplay rhythmTextDisplay(500);
  rhythmTextDisplay.SetDimension(Dimension(8, 4));
  rhythmTextDisplay.SetRenderFunc([&](Point origin) -> void {
    Color color = arpConfigColor[ARP_RHYTHM];

    // r
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);

    // H
    MatrixOS::LED::SetColor(origin + Point(2, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(2, 3), Color::White);
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 2), Color::White);
    MatrixOS::LED::SetColor(origin + Point(4, 3), Color::White);

    // Y
    MatrixOS::LED::SetColor(origin + Point(5, 0), color);
    MatrixOS::LED::SetColor(origin + Point(6, 1), color);
    MatrixOS::LED::SetColor(origin + Point(6, 2), color);
    MatrixOS::LED::SetColor(origin + Point(6, 3), color);
    MatrixOS::LED::SetColor(origin + Point(7, 0), color);
  });
  rhythmTextDisplay.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_RHYTHM; });
  arpConfigMenu.AddUIComponent(rhythmTextDisplay, Point(0, 2));

  RhythmVisualizer rhythmVisualizer(arpConfigColor[ARP_RHYTHM], Color(0x8000FF), &runtimes[activeConfig].arpeggiator);
  rhythmVisualizer.SetEnableFunc([&]() -> bool { return (arpMenuPage == ARP_RHYTHM) && !rhythmTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(rhythmVisualizer, Point(0, 2));

  // Euclidean Lengths Decrease Button
  UIButton euclideanLengthsDecreaseBtn;
  euclideanLengthsDecreaseBtn.SetName("Rhythm Lengths -1");
  euclideanLengthsDecreaseBtn.SetSize(Dimension(1, 1));
  euclideanLengthsDecreaseBtn.SetColorFunc([&]() -> Color {
    return Color(0xFF0000).DimIfNot(notePadConfigs[activeConfig].arpConfig.euclideanLengths > 1);
  });
  euclideanLengthsDecreaseBtn.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_RHYTHM; });
  euclideanLengthsDecreaseBtn.OnPress([&]() -> void {
    if (notePadConfigs[activeConfig].arpConfig.euclideanLengths > 1) {
      notePadConfigs[activeConfig].arpConfig.euclideanLengths--;
      // Cap euclideanSteps to new length if needed
      if (notePadConfigs[activeConfig].arpConfig.euclideanSteps > notePadConfigs[activeConfig].arpConfig.euclideanLengths) {
        notePadConfigs[activeConfig].arpConfig.euclideanSteps = notePadConfigs[activeConfig].arpConfig.euclideanLengths;
      }
      configModified = true;
      runtimes[activeConfig].arpeggiator.UpdateConfig();
    }
  });
  arpConfigMenu.AddUIComponent(euclideanLengthsDecreaseBtn, Point(0, 7));

  // Euclidean Lengths Increase Button
  UIButton euclideanLengthsIncreaseBtn;
  euclideanLengthsIncreaseBtn.SetName("Rhythm Lengths +1");
  euclideanLengthsIncreaseBtn.SetSize(Dimension(1, 1));
  euclideanLengthsIncreaseBtn.SetColorFunc([&]() -> Color {
    return Color(0x00FF00).DimIfNot(notePadConfigs[activeConfig].arpConfig.euclideanLengths < 32);
  });
  euclideanLengthsIncreaseBtn.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_RHYTHM; });
  euclideanLengthsIncreaseBtn.OnPress([&]() -> void {
    if (notePadConfigs[activeConfig].arpConfig.euclideanLengths < 32) {
      notePadConfigs[activeConfig].arpConfig.euclideanLengths++;
      configModified = true;
      runtimes[activeConfig].arpeggiator.UpdateConfig();
    }
  });
  arpConfigMenu.AddUIComponent(euclideanLengthsIncreaseBtn, Point(1, 7));

  // Euclidean Steps Decrease Button
  UIButton euclideanStepsDecreaseBtn;
  euclideanStepsDecreaseBtn.SetName("Rhythm Steps -1");
  euclideanStepsDecreaseBtn.SetSize(Dimension(1, 1));
  euclideanStepsDecreaseBtn.SetColorFunc([&]() -> Color {
    return Color(0xFF00FF).DimIfNot(notePadConfigs[activeConfig].arpConfig.euclideanSteps > 1);
  });
  euclideanStepsDecreaseBtn.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_RHYTHM; });
  euclideanStepsDecreaseBtn.OnPress([&]() -> void {
    if (notePadConfigs[activeConfig].arpConfig.euclideanSteps > 1) {
      notePadConfigs[activeConfig].arpConfig.euclideanSteps--;
      configModified = true;
      runtimes[activeConfig].arpeggiator.UpdateConfig();
    }
  });
  arpConfigMenu.AddUIComponent(euclideanStepsDecreaseBtn, Point(6, 7));

  // Euclidean Steps Increase Button
  UIButton euclideanStepsIncreaseBtn;
  euclideanStepsIncreaseBtn.SetName("Rhythm Steps +1");
  euclideanStepsIncreaseBtn.SetSize(Dimension(1, 1));
  euclideanStepsIncreaseBtn.SetColorFunc([&]() -> Color {
    return Color(0x00FFFF).DimIfNot(notePadConfigs[activeConfig].arpConfig.euclideanSteps < notePadConfigs[activeConfig].arpConfig.euclideanLengths);
  });
  euclideanStepsIncreaseBtn.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_RHYTHM; });
  euclideanStepsIncreaseBtn.OnPress([&]() -> void {
    if (notePadConfigs[activeConfig].arpConfig.euclideanSteps < notePadConfigs[activeConfig].arpConfig.euclideanLengths) {
      notePadConfigs[activeConfig].arpConfig.euclideanSteps++;
      configModified = true;
      runtimes[activeConfig].arpeggiator.UpdateConfig();
    }
  });
  arpConfigMenu.AddUIComponent(euclideanStepsIncreaseBtn, Point(7, 7));
  

  // Reset button
  UIButton resetBtn;
  resetBtn.SetName("Reset");
  resetBtn.SetColor(Color(0xFF0000));
  resetBtn.SetColorFunc([&]() -> Color {
    return Color(0xFF0000).DimIfNot(arpMenuPage != ARP_DIRECTION);
  });
  resetBtn.SetEnableFunc([&]() -> bool { return arpMenuPage != ARP_DIRECTION; });
  resetBtn.OnPress([&]() -> void {
    switch(arpMenuPage) {
      case ARP_BPM:
        bpmValue = 120;
        bpm = 120;
        midiClock.SetBPM(120);
        break;
      case ARP_SWING:
        swingValue = 50;
        configModified = true;
        notePadConfigs[activeConfig].arpConfig.swing = 50;
        runtimes[activeConfig].arpeggiator.UpdateConfig();
        break;
      case ARP_GATE:
        gateValue = 50;
        configModified = true;
        notePadConfigs[activeConfig].arpConfig.gateTime = 50;
        runtimes[activeConfig].arpeggiator.UpdateConfig();
        break;
      case ARP_STEP:
        stepValue = 1;
        configModified = true;
        notePadConfigs[activeConfig].arpConfig.step = 1;
        runtimes[activeConfig].arpeggiator.UpdateConfig();
        break;
      case ARP_STEP_OFFSET:
        stepOffsetValue = 12;
        stepOffsetDisplayValue = 12;
        configModified = true;
        notePadConfigs[activeConfig].arpConfig.stepOffset = 12;
        runtimes[activeConfig].arpeggiator.UpdateConfig();
        break;
      case ARP_REPEAT:
        repeatValue = 0;
        configModified = true;
        notePadConfigs[activeConfig].arpConfig.repeat = 0;
        runtimes[activeConfig].arpeggiator.UpdateConfig();
        break;
      case ARP_RHYTHM:
        configModified = true;
        notePadConfigs[activeConfig].arpConfig.euclideanLengths = 16;
        notePadConfigs[activeConfig].arpConfig.euclideanSteps = 16;
        notePadConfigs[activeConfig].arpConfig.euclideanOffset = 0;
        runtimes[activeConfig].arpeggiator.UpdateConfig();
        break;
    }
  });
  arpConfigMenu.AddUIComponent(resetBtn, Point(7, 6));

  // Infinity button (for Gate and Repeat)
  UIButton infBtn;
  infBtn.SetName("Inf");
  infBtn.SetColorFunc([&]() -> Color {
    if (arpMenuPage == ARP_GATE) {
      return Color::White.DimIfNot(notePadConfigs[activeConfig].arpConfig.gateTime == 0);
    } else if (arpMenuPage == ARP_REPEAT) {
      return Color::White.DimIfNot(notePadConfigs[activeConfig].arpConfig.repeat == 0);
    }
    return Color::White.Dim();
  });
  infBtn.SetEnableFunc([&]() -> bool { return arpMenuPage == ARP_GATE || arpMenuPage == ARP_REPEAT; });
  infBtn.OnPress([&]() -> void {
    if (arpMenuPage == ARP_GATE) {
      gateValue = 0;
      configModified = true;
      notePadConfigs[activeConfig].arpConfig.gateTime = 0;
      runtimes[activeConfig].arpeggiator.UpdateConfig();
    } else if (arpMenuPage == ARP_REPEAT) {
      repeatValue = 0;
      configModified = true;
      notePadConfigs[activeConfig].arpConfig.repeat = 0;
      runtimes[activeConfig].arpeggiator.UpdateConfig();
    }
  });
  arpConfigMenu.AddUIComponent(infBtn, Point(0, 6));

  arpConfigMenu.Start();

  if (configModified) {
    SaveConfigs();
  }
}

void Note::SaveConfigs() {
  MatrixOS::NVS::SetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs));
}

void Note::Tick() {
  if(midiClock.Tick())
  {
    if(clockMode == CLOCK_INTERNAL_CLOCKOUT && midiClock.TickCount() % (EFFECT_TPQN / 24) == 0)
    {
      MatrixOS::MIDI::Send(MidiPacket::Clock(), MIDI_PORT_ALL);
    }
    runtimes[0].Tick();
    runtimes[1].Tick();
  }
}
