#include "Note.h"
#include "OctaveShifter.h"
#include "ScaleVisualizer.h"
#include "UnderglowLight.h"
#include "NoteControlBar.h"
#include "ArpDirVisualizer.h"
#include "TimedDisplay.h"

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
      MatrixOS::NVS::SetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs));
    }
  }
  else
  { 
    MatrixOS::NVS::SetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs)); \
    nvsVersion = NOTE_APP_VERSION;
  }

  activeConfig.Get(); //Load it first

  // Set up the Action Menu UI ---------------------------------------------------------------------
  UI actionMenu("Action Menu", Color(0x00FFFF), false);

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
    forceSensitiveToggle.SetColorFunc([&]() -> Color { return  Color(0x00FFB0).DimIfNot(notePadConfigs[activeConfig].forceSensitive); });
    forceSensitiveToggle.OnPress([&]() -> void { notePadConfigs[activeConfig].forceSensitive = !notePadConfigs[activeConfig].forceSensitive; });
    forceSensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll(forceSensitiveToggle.GetName() + " " + (notePadConfigs[activeConfig].forceSensitive ? "On" : "Off"), forceSensitiveToggle.GetColor()); });
  }
  else
  {
    forceSensitiveToggle.SetColor(Color(0x00FFB0).Dim());
    forceSensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll("Velocity Sensitivity Not Supported", Color(0x00FFB0)); });
  }
  actionMenu.AddUIComponent(forceSensitiveToggle, Point(7, 5));

  OctaveShifter octaveShifter(8, notePadConfigs, &activeConfig.value);
  actionMenu.AddUIComponent(octaveShifter, Point(0, 0));

  // Split View
  UIButton splitViewToggle;
  splitViewToggle.SetName("Split View");
  splitViewToggle.SetColorFunc([&]() -> Color { 
    switch(splitView)
    {

      case VERT_SPLIT: return Color(0x00FFFF);
      case HORIZ_SPLIT: return Color(0xFF00FF);
      default: return Color(0xFFFFFF).Dim();
    }
  });
  splitViewToggle.OnPress([&]() -> void { splitView = (ESpiltView)(((uint8_t)splitView + 1) % 3); });
  splitViewToggle.OnHold([&]() -> void { 
    switch(splitView)
    {
      case SINGLE_VIEW: MatrixOS::UIUtility::TextScroll("Single View", Color(0xFFFFFF)); break;
      case VERT_SPLIT: MatrixOS::UIUtility::TextScroll("Vertical Split", Color(0x00FFFF)); break;
      case HORIZ_SPLIT: MatrixOS::UIUtility::TextScroll("Horizontal Split", Color(0xFF00FF)); break;
    }
  });
  actionMenu.AddUIComponent(splitViewToggle, Point(1, 0));

  UIButton notepad1SelectBtn;
  notepad1SelectBtn.SetName("Note Pad 1");
  notepad1SelectBtn.SetColorFunc([&]() -> Color { return notePadConfigs[0].color.DimIfNot(activeConfig.Get() == 0); });
  notepad1SelectBtn.OnPress([&]() -> void { activeConfig = 0; });
  actionMenu.AddUIComponent(notepad1SelectBtn, Point(3, 0));

  UIButton notepad2SelectBtn;
  notepad2SelectBtn.SetName("Note Pad 2");
  notepad2SelectBtn.SetColorFunc([&]() -> Color { return notePadConfigs[1].color.DimIfNot(activeConfig.Get() == 1); });
  notepad2SelectBtn.OnPress([&]() -> void { activeConfig = 1; });
  actionMenu.AddUIComponent(notepad2SelectBtn, Point(4, 0));

  UIButton notepadColorBtn;
  notepadColorBtn.SetName("Note Pad Color");
  notepadColorBtn.SetColorFunc([&]() -> Color { return notePadConfigs[activeConfig].color; });
  notepadColorBtn.OnPress([&]() -> void { ColorSelector(); });
  actionMenu.AddUIComponent(notepadColorBtn, Point(7, 0));

  // Other Controls
  UIToggle controlBarToggle;
  controlBarToggle.SetName("Control Bar");
  controlBarToggle.SetColor(Color(0xFF8000));
  controlBarToggle.SetValuePointer(&controlBar);
  controlBarToggle.OnPress([&]() -> void {controlBar.Save();});
  actionMenu.AddUIComponent(controlBarToggle, Point(1, 7));
  
  // Other Controls
  UIButton arpConfigBtn;
  arpConfigBtn.SetName("Arpeggiator Config");
  arpConfigBtn.SetColor(Color(0x80FF00));
  arpConfigBtn.OnPress([&]() -> void {ArpConfigMenu();});
  arpConfigBtn.SetEnableFunc([&]() -> bool {return controlBar;});
  actionMenu.AddUIComponent(arpConfigBtn, Point(3, 7));

  // Other Controls
  UIButton systemSettingBtn;
  systemSettingBtn.SetName("System Setting");
  systemSettingBtn.SetColor(Color(0xFFFFFF));
  systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });
  actionMenu.AddUIComponent(systemSettingBtn, Point(7, 7));

  actionMenu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == HOLD)
      { Exit(); }
      else if (keyEvent->info.state == RELEASED)
      {
        MatrixOS::NVS::SetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs));
        PlayView();
      }
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
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
  

  // Create NotePadRuntime structures
  NotePadRuntime NotePadRuntime1;
  NotePadRuntime1.config = &notePadConfigs[activeConfig.Get() == 1];

  NotePadRuntime NotePadRuntime2;
  NotePadRuntime2.config = &notePadConfigs[activeConfig.Get() == 0];

  NotePad notePad1(padSize, &NotePadRuntime1);
  playView.AddUIComponent(notePad1, Point(0, 0));

  UnderglowLight underglow1(underglowSize, notePadConfigs[activeConfig.Get() == 1].color);
  playView.AddUIComponent(underglow1, Point(-1, -1));

  NotePad notePad2(padSize, &NotePadRuntime2);
  UnderglowLight underglow2(underglowSize, notePadConfigs[activeConfig.Get() == 0].color);
  
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

  playView.SetLoopFunc([&]() -> void {
    notePad1.Tick();
    notePad2.Tick();
  });

  playView.Start();
}

void Note::ScaleSelector() {
  UI scaleSelector("Scale Selector", Color(0xFF0090), false);

  ScaleVisualizer scaleVisualizer(&notePadConfigs[activeConfig].rootKey, &notePadConfigs[activeConfig].scale, notePadConfigs[activeConfig].color,
                                  notePadConfigs[activeConfig].rootColor);
  scaleSelector.AddUIComponent(scaleVisualizer, Point(0, 0));

  UIItemSelector<uint16_t> scaleSelectorBar;
  scaleSelectorBar.SetDimension(Dimension(8, 4));
  scaleSelectorBar.SetColor(Color(0xFF0090));
  scaleSelectorBar.SetItemPointer(&notePadConfigs[activeConfig].scale);
  scaleSelectorBar.SetCount(32);
  scaleSelectorBar.SetItems(scales);
  scaleSelectorBar.SetIndividualNameFunc([&](uint16_t index) -> string { return scale_names[index]; });
  scaleSelector.AddUIComponent(scaleSelectorBar, Point(0, 4));

  scaleSelector.Start();
}

void Note::ColorSelector() {
  UI colorSelector("Color Selector", notePadConfigs[activeConfig].color, false);
  uint8_t page = 0;  // 0 = Preset, 1 = Customize

  // Create NotePadRuntime structure for color selector
  NotePadRuntime colorSelectorData;
  colorSelectorData.config = &notePadConfigs[activeConfig];

  NotePad notePad(Dimension(8, 4), &colorSelectorData);
  colorSelector.AddUIComponent(notePad, Point(0, 0));

  UIButton presetsBtn;
  presetsBtn.SetName("Preset");
  presetsBtn.SetColorFunc([&]() -> Color {
    return Color(0xFFFFFF).DimIfNot(page == 0);
  });
  presetsBtn.OnPress([&]() -> void { page = 0; });
  colorSelector.AddUIComponent(presetsBtn, Point(0, 5));

  UIButton customizeBtn;
  customizeBtn.SetName("Customize");
  customizeBtn.SetColorFunc([&]() -> Color {
    return Color(0xFFFFFF).DimIfNot(page == 1);
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
  });
  rootColorSelectorBtn.SetEnableFunc([&]() -> bool { return page == 1; });
  colorSelector.AddUIComponent(rootColorSelectorBtn, Point(2, 5));

  UIButton notePadColorSelectorBtn;
  notePadColorSelectorBtn.SetName("Note Pad Color");
  notePadColorSelectorBtn.SetColorFunc([&]() -> Color { return notePadConfigs[activeConfig].color; });
  notePadColorSelectorBtn.SetSize(Dimension(2, 2));
  notePadColorSelectorBtn.OnPress([&]() -> void {
    MatrixOS::UIUtility::ColorPicker(notePadConfigs[activeConfig].color);
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
    notePadConfigs[activeConfig].useWhiteAsOutOfScale = !notePadConfigs[activeConfig].useWhiteAsOutOfScale;
  });
  colorSelector.AddUIComponent(whiteOutOfScaleToggle, Point(7, 5));

  colorSelector.Start();
}

void Note::LayoutSelector() {
  UI layoutSelector("Layout Selector", Color(0xFFFF00), false);

  int32_t x_offset = notePadConfigs[activeConfig].x_offset;
  int32_t y_offset = notePadConfigs[activeConfig].y_offset;

  UIButton octaveModeBtn;
  octaveModeBtn.SetName("Octave Mode");
  octaveModeBtn.SetColorFunc([&]() -> Color { return Color(0xFFFF00).DimIfNot(notePadConfigs[activeConfig].mode == OCTAVE_LAYOUT); });
  octaveModeBtn.OnPress([&]() -> void { notePadConfigs[activeConfig].mode = OCTAVE_LAYOUT; });
  layoutSelector.AddUIComponent(octaveModeBtn, Point(2, 0));

  UIButton offsetModeBtn;
  offsetModeBtn.SetName("Offset Mode");
  offsetModeBtn.SetColorFunc([&]() -> Color { return Color(0xFFFF00).DimIfNot(notePadConfigs[activeConfig].mode == OFFSET_LAYOUT); });
  offsetModeBtn.OnPress([&]() -> void { 
    if(notePadConfigs[activeConfig].mode != OFFSET_LAYOUT)
    {
      notePadConfigs[activeConfig].mode = OFFSET_LAYOUT;
      x_offset = 1;
      y_offset = 3;
    }
  });
  layoutSelector.AddUIComponent(offsetModeBtn, Point(3, 0));

  UIButton chromaticModeBtn;
  chromaticModeBtn.SetName("Chromatic Mode");
  chromaticModeBtn.SetColorFunc([&]() -> Color { return Color(0xFFFF00).DimIfNot(notePadConfigs[activeConfig].mode == CHROMATIC_LAYOUT); });
  chromaticModeBtn.OnPress([&]() -> void { notePadConfigs[activeConfig].mode = CHROMATIC_LAYOUT; });
  layoutSelector.AddUIComponent(chromaticModeBtn, Point(4, 0));

  UIButton pianoModeBtn;
  pianoModeBtn.SetName("Piano Mode");
  pianoModeBtn.SetColorFunc([&]() -> Color { return Color(0xFFFF00).DimIfNot(notePadConfigs[activeConfig].mode == PIANO_LAYOUT); });
  pianoModeBtn.OnPress([&]() -> void { notePadConfigs[activeConfig].mode = PIANO_LAYOUT; });
  layoutSelector.AddUIComponent(pianoModeBtn, Point(5, 0));

  // Offset Mode
  UISelector yOffsetInput;
  yOffsetInput.SetDimension(Dimension(1, 8));
  yOffsetInput.SetName("Y Offset");
  yOffsetInput.SetColor(Color(0xFF00FF));
  yOffsetInput.SetValuePointer((uint16_t*)&y_offset);
  yOffsetInput.SetLitMode(UISelectorLitMode::LIT_LESS_EQUAL_THAN);
  yOffsetInput.SetDirection(UISelectorDirection::UP_THEN_RIGHT);
  yOffsetInput.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(yOffsetInput, Point(0, 0));

  UI4pxNumber yOffsetDisplay;
  yOffsetDisplay.SetColor(Color(0xFF00FF));
  yOffsetDisplay.SetDigits(1);
  yOffsetDisplay.SetValuePointer((int32_t*)&y_offset);
  yOffsetDisplay.SetAlternativeColor(Color(0xFF00FF));
  yOffsetDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(yOffsetDisplay, Point(2, 2));

  UISelector xOffsetInput;
  xOffsetInput.SetDimension(Dimension(8, 1));
  xOffsetInput.SetName("X Offset");
  xOffsetInput.SetColor(Color(0x00FFFF));
  xOffsetInput.SetValuePointer((uint16_t*)&x_offset);
  xOffsetInput.SetLitMode(UISelectorLitMode::LIT_LESS_EQUAL_THAN);
  xOffsetInput.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(xOffsetInput, Point(0, 7));

  UI4pxNumber xOffsetDisplay;
  xOffsetDisplay.SetColor(Color(0x00FFFF));
  xOffsetDisplay.SetDigits(1);
  xOffsetDisplay.SetValuePointer((int32_t*)&x_offset);
  xOffsetDisplay.SetAlternativeColor(Color(0x00FFFF));
  xOffsetDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(xOffsetDisplay, Point(5, 2));

  // Show Off Scale Notes Toggle (only for Octave and Offset modes)
  UIButton inKeyNoteOnlyToggle;
  inKeyNoteOnlyToggle.SetName("In Key Notes Only");
  inKeyNoteOnlyToggle.SetColorFunc([&]() -> Color { return Color(0xFFFFFF).DimIfNot(notePadConfigs[activeConfig].inKeyNoteOnly); });
  inKeyNoteOnlyToggle.OnPress([&]() -> void { notePadConfigs[activeConfig].inKeyNoteOnly = !notePadConfigs[activeConfig].inKeyNoteOnly; });
  inKeyNoteOnlyToggle.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OCTAVE_LAYOUT || notePadConfigs[activeConfig].mode == OFFSET_LAYOUT || notePadConfigs[activeConfig].mode == CHROMATIC_LAYOUT;; });
  layoutSelector.AddUIComponent(inKeyNoteOnlyToggle, Point(0, 7));

  layoutSelector.Start();

  if(notePadConfigs[activeConfig].mode == OFFSET_LAYOUT)
  {
    notePadConfigs[activeConfig].x_offset = x_offset;
    notePadConfigs[activeConfig].y_offset = y_offset;
  }
}

void Note::ChannelSelector() {
  UI channelSelector("Channel Selector", Color(0x60FF00), false);

  int32_t offsettedChannel = notePadConfigs[activeConfig].channel + 1;
  UI4pxNumber numDisplay;
  numDisplay.SetColor(Color(0x60FF00));
  numDisplay.SetDigits(2);
  numDisplay.SetValuePointer(&offsettedChannel);
  numDisplay.SetAlternativeColor(Color(0xFFFFFF));
  numDisplay.SetSpacing(1);
  channelSelector.AddUIComponent(numDisplay, Point(1, 0));

  UISelector channelInput;
  channelInput.SetDimension(Dimension(8, 2));
  channelInput.SetName("Channel");
  channelInput.SetColor(Color(0x60FF00));
  channelInput.SetCount(16);
  channelInput.SetValuePointer((uint16_t*)&notePadConfigs[activeConfig].channel);
  channelInput.OnChange([&](uint16_t val) -> void { offsettedChannel = val + 1; });

  channelSelector.AddUIComponent(channelInput, Point(0, 6));

  channelSelector.Start();
}

void Note::ArpConfigMenu() {
  UI arpConfigMenu("Arpeggiator Config", Color(0x80FF00));
  uint64_t menuOpenTime = MatrixOS::SYS::Millis();

  enum ArpConfigType:uint16_t
  {
    ARP_BPM,
    ARP_SWING,
    ARP_GATE,
    ARP_DIRECTION,
    ARP_STEP,
    ARP_STEP_OFFSET,
  };

  ArpConfigType page = ARP_BPM;

  Color arpConfigColor[6]
  {
    Color(0xFF0000), // Red - BPM
    Color(0xFF8000), // Orange - Swing
    Color(0xFFFF00), // Yellow - Gate
    Color(0x00FF00), // Green - Direction
    Color(0x00FFFF), // Cyan - Step
    Color(0x4000FF), // Blue - Step Offset
  };

  string arpConfigName[6]
  {
    "BPM",
    "Swing",
    "Gate",
    "Direction",
    "Step",
    "Step Offset",
  };

  // Shared arrays for all number modifiers
  int32_t coarseModifier[8] = {-50, -20, -5, -1, 1, 5, 20, 50};
  int32_t fineModifier[8] = {-25, -10, -5, -1, 1, 5, 10, 25};
  uint8_t modifierGradient[8] = {255, 127, 64, 32, 32, 64, 127, 255};

  UISelector arpConfigSelector;
  arpConfigSelector.SetCount(6);
  arpConfigSelector.SetDimension(Dimension(6, 1));
  arpConfigSelector.SetIndividualColorFunc([&](uint16_t index) -> Color { return arpConfigColor[index]; });
  arpConfigSelector.SetIndividualNameFunc([&](uint16_t index) -> string { return arpConfigName[index]; });
  arpConfigSelector.SetValuePointer((uint16_t*)&page);
  arpConfigSelector.OnChange([&](uint16_t val) -> void { if(page != (ArpConfigType)val) {page = (ArpConfigType)val; menuOpenTime = MatrixOS::SYS::Millis();}});
  arpConfigMenu.AddUIComponent(arpConfigSelector, Point(1, 0));

  // BPM selector
  TimedDisplay bpmTextDisplay(1000);
  bpmTextDisplay.SetDimension(Dimension(8, 4));
  bpmTextDisplay.SetRenderFunc([&](Point origin) -> void {
    Color color = arpConfigColor[ARP_BPM];

    // B
    MatrixOS::LED::SetColor(origin + Point(0, 0), color);
    MatrixOS::LED::SetColor(origin + Point(0, 1), color);
    MatrixOS::LED::SetColor(origin + Point(0, 2), color);
    MatrixOS::LED::SetColor(origin + Point(0, 3), color);
    MatrixOS::LED::SetColor(origin + Point(1, 0), color);
    MatrixOS::LED::SetColor(origin + Point(1, 1), color);
    MatrixOS::LED::SetColor(origin + Point(1, 3), color);
    MatrixOS::LED::SetColor(origin + Point(2, 1), color);
    MatrixOS::LED::SetColor(origin + Point(2, 2), color);
    MatrixOS::LED::SetColor(origin + Point(2, 3), color);

    // P
    MatrixOS::LED::SetColor(origin + Point(3, 0), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(3, 1), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(3, 2), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(3, 3), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(4, 0), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(origin + Point(4, 1), Color(0xFFFFFF));

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
  bpmTextDisplay.SetEnableFunc([&]() -> bool { return page == ARP_BPM; });
  arpConfigMenu.AddUIComponent(bpmTextDisplay, Point(0, 2));


  UI4pxNumber bpmDisplay;
  bpmDisplay.SetColor(arpConfigColor[ARP_BPM]);
  bpmDisplay.SetDigits(3);
  bpmDisplay.SetValuePointer((int32_t*)&notePadConfigs[activeConfig].arpConfig.bpm);
  bpmDisplay.SetAlternativeColor(Color(0xFFFFFF));
  bpmDisplay.SetEnableFunc([&]() -> bool { return (page == ARP_BPM) &&  !bpmTextDisplay.IsEnabled(); });
  arpConfigMenu.AddUIComponent(bpmDisplay, Point(-1, 2));

  UINumberModifier bpmNumberModifier;
  bpmNumberModifier.SetColor(arpConfigColor[ARP_BPM]);
  bpmNumberModifier.SetLength(8);
  bpmNumberModifier.SetValuePointer((int32_t*)&notePadConfigs[activeConfig].arpConfig.bpm);
  bpmNumberModifier.SetModifiers(coarseModifier);
  bpmNumberModifier.SetControlGradient(modifierGradient);
  bpmNumberModifier.SetLowerLimit(20);
  bpmNumberModifier.SetUpperLimit(299);
  bpmNumberModifier.SetEnableFunc([&]() -> bool { return page == ARP_BPM; });
  bpmNumberModifier.OnChange([&](int32_t val) -> void { bpmTextDisplay.Disable(); });
  arpConfigMenu.AddUIComponent(bpmNumberModifier, Point(0, 7));

  // Swing selector
  UI4pxNumber swingDisplay;
  swingDisplay.SetColor(arpConfigColor[ARP_SWING]);
  swingDisplay.SetDigits(3);
  swingDisplay.SetValuePointer((int32_t*)&notePadConfigs[activeConfig].arpConfig.swingAmount);
  swingDisplay.SetAlternativeColor(Color(0xFFFFFF));
  swingDisplay.SetEnableFunc([&]() -> bool { return page == ARP_SWING; });
  arpConfigMenu.AddUIComponent(swingDisplay, Point(-1, 2));

  // Custom modifier for swing (smaller increments)
  int32_t swingModifier[8] = {-10, -5, -2, -1, 1, 2, 5, 10};
  UINumberModifier swingNumberModifier;
  swingNumberModifier.SetColor(arpConfigColor[ARP_SWING]);
  swingNumberModifier.SetLength(8);
  swingNumberModifier.SetValuePointer((int32_t*)&notePadConfigs[activeConfig].arpConfig.swingAmount);
  swingNumberModifier.SetModifiers(swingModifier);
  swingNumberModifier.SetControlGradient(modifierGradient);
  swingNumberModifier.SetLowerLimit(20);
  swingNumberModifier.SetUpperLimit(80);
  swingNumberModifier.SetEnableFunc([&]() -> bool { return page == ARP_SWING; });
  arpConfigMenu.AddUIComponent(swingNumberModifier, Point(0, 7));

  // Gate selector
  UI4pxNumber gateDisplay;
  gateDisplay.SetColor(arpConfigColor[ARP_GATE]);
  gateDisplay.SetDigits(3);
  gateDisplay.SetValuePointer((int32_t*)&notePadConfigs[activeConfig].arpConfig.gateTime);
  gateDisplay.SetAlternativeColor(Color(0xFFFFFF));
  gateDisplay.SetEnableFunc([&]() -> bool { return page == ARP_GATE; });
  arpConfigMenu.AddUIComponent(gateDisplay, Point(-1, 2));

  UINumberModifier gateNumberModifier;
  gateNumberModifier.SetColor(arpConfigColor[ARP_GATE]);
  gateNumberModifier.SetLength(8);
  gateNumberModifier.SetValuePointer((int32_t*)&notePadConfigs[activeConfig].arpConfig.gateTime);
  gateNumberModifier.SetModifiers(fineModifier);
  gateNumberModifier.SetControlGradient(modifierGradient);
  gateNumberModifier.SetLowerLimit(0);
  gateNumberModifier.SetUpperLimit(200);
  gateNumberModifier.SetEnableFunc([&]() -> bool { return page == ARP_GATE; });
  arpConfigMenu.AddUIComponent(gateNumberModifier, Point(0, 7));

  // Direction Selector
  ArpDirVisualizer arpDirVisualizer(&notePadConfigs[activeConfig].arpConfig.direction, arpConfigColor[ARP_DIRECTION]);
  arpDirVisualizer.SetEnableFunc([&]() -> bool { return page == ARP_DIRECTION; });
  arpConfigMenu.AddUIComponent(arpDirVisualizer, Point(0, 2));

  UISelector directionSelector;
  directionSelector.SetDimension(Dimension(8, 4));
  directionSelector.SetColor(arpConfigColor[ARP_DIRECTION]);
  directionSelector.SetValueFunc([&]() -> uint16_t { return (uint16_t)notePadConfigs[activeConfig].arpConfig.direction; });
  directionSelector.OnChange([&](uint16_t value) -> void { notePadConfigs[activeConfig].arpConfig.direction = (ArpDirection)value; });
  directionSelector.SetCount(16);
  directionSelector.SetIndividualNameFunc([&](uint16_t index) -> string { return arpDirectionNames[index]; });
  directionSelector.SetEnableFunc([&]() -> bool { return page == ARP_DIRECTION; });
  arpConfigMenu.AddUIComponent(directionSelector, Point(0, 6));

  // Step selector
  UI4pxNumber stepDisplay;
  stepDisplay.SetColor(arpConfigColor[ARP_STEP]);
  stepDisplay.SetDigits(3);
  stepDisplay.SetValuePointer((int32_t*)&notePadConfigs[activeConfig].arpConfig.step);
  stepDisplay.SetAlternativeColor(Color(0xFFFFFF));
  stepDisplay.SetEnableFunc([&]() -> bool { return page == ARP_STEP; });
  arpConfigMenu.AddUIComponent(stepDisplay, Point(-1, 2));

  // Custom modifier for step (1-8 range)

  UINumberModifier stepNumberModifier;
  stepNumberModifier.SetColor(arpConfigColor[ARP_STEP]);
  stepNumberModifier.SetLength(8);
  stepNumberModifier.SetValuePointer((int32_t*)&notePadConfigs[activeConfig].arpConfig.step);
  stepNumberModifier.SetModifiers(fineModifier);
  stepNumberModifier.SetControlGradient(modifierGradient);
  stepNumberModifier.SetLowerLimit(1);
  stepNumberModifier.SetUpperLimit(8);
  stepNumberModifier.SetEnableFunc([&]() -> bool { return page == ARP_STEP; });
  arpConfigMenu.AddUIComponent(stepNumberModifier, Point(0, 7));

  // Step Offset selector (with minus sign support)
  int32_t stepOffsetValue = notePadConfigs[activeConfig].arpConfig.stepOffset;
  int32_t stepOffsetDisplayValue = abs(stepOffsetValue);

  UIButton stepOffsetMinusSign;
  stepOffsetMinusSign.SetColor(arpConfigColor[ARP_STEP_OFFSET]);
  stepOffsetMinusSign.SetSize(Dimension(2, 1));
  stepOffsetMinusSign.SetEnableFunc([&]() -> bool { return page == ARP_STEP_OFFSET && stepOffsetValue < 0; });
  arpConfigMenu.AddUIComponent(stepOffsetMinusSign, Point(0, 4));

  UI4pxNumber stepOffsetDisplay;
  stepOffsetDisplay.SetColor(arpConfigColor[ARP_STEP_OFFSET]);
  stepOffsetDisplay.SetDigits(2);
  stepOffsetDisplay.SetValuePointer((int32_t*)&stepOffsetDisplayValue);
  stepOffsetDisplay.SetAlternativeColor(Color(0xFFFFFF));
  stepOffsetDisplay.SetEnableFunc([&]() -> bool { return page == ARP_STEP_OFFSET; });
  arpConfigMenu.AddUIComponent(stepOffsetDisplay, Point(2, 2));

  // Custom modifier for step offset (-24 to 24 semitones)
  UINumberModifier stepOffsetNumberModifier;
  stepOffsetNumberModifier.SetColor(arpConfigColor[ARP_STEP_OFFSET]);
  stepOffsetNumberModifier.SetLength(8);
  stepOffsetNumberModifier.SetValuePointer(&stepOffsetValue);
  stepOffsetNumberModifier.SetModifiers(fineModifier);
  stepOffsetNumberModifier.SetControlGradient(modifierGradient);
  stepOffsetNumberModifier.SetLowerLimit(-24);
  stepOffsetNumberModifier.SetUpperLimit(24);
  stepOffsetNumberModifier.OnChange([&](int32_t value) -> void {
    stepOffsetValue = value;
    notePadConfigs[activeConfig].arpConfig.stepOffset = (int8_t)value;
    stepOffsetDisplayValue = abs(value);
  });
  stepOffsetNumberModifier.SetEnableFunc([&]() -> bool { return page == ARP_STEP_OFFSET; });
  arpConfigMenu.AddUIComponent(stepOffsetNumberModifier, Point(0, 7));

  arpConfigMenu.Start();
}

