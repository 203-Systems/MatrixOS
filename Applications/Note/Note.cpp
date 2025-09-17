#include "Note.h"
#include "OctaveShifter.h"
#include "ScaleVisualizer.h"
#include "UnderglowLight.h"

void Note::Setup(va_list args) {
  // Set up / Load configs --------------------------------------------------------------------------

  // Default Values
  notePadConfigs[1].color = Color(0xFF00FF);
  notePadConfigs[1].rootColor = Color(0x8800FF);

  // Load From NVS
  if (nvsVersion == (uint32_t)NOTE_APP_VERSION)
  { 
    MatrixOS::NVS::GetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs)); 
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

  UIButton velocitySensitiveToggle;
  velocitySensitiveToggle.SetName("Velocity Sensitive");
  if(Device::KeyPad::velocity_sensitivity)
  {
    velocitySensitiveToggle.SetColorFunc([&]() -> Color { return  Color(0x00FFB0).DimIfNot(notePadConfigs[activeConfig].velocitySensitive); });
    velocitySensitiveToggle.OnPress([&]() -> void { notePadConfigs[activeConfig].velocitySensitive = !notePadConfigs[activeConfig].velocitySensitive; });
    velocitySensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll(velocitySensitiveToggle.GetName() + " " + (notePadConfigs[activeConfig].velocitySensitive ? "On" : "Off"), velocitySensitiveToggle.GetColor()); });
  }
  else
  {
    velocitySensitiveToggle.SetColor(Color(0x00FFB0).Dim());
    velocitySensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll("Velocity Sensitivity Not Supported", Color(0x00FFB0)); });
  }
  actionMenu.AddUIComponent(velocitySensitiveToggle, Point(7, 5));

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
      padSize = Dimension(8, 8);
      underglowSize = Dimension(10, 10);
      break;
    case VERT_SPLIT:
      padSize = Dimension(4, 8);
      underglowSize = Dimension(5, 10);
      break;
    case HORIZ_SPLIT:
      padSize = Dimension(8, 4);
      underglowSize = Dimension(10, 5);
      break;
  }
  

  NotePad notePad1(padSize, &notePadConfigs[activeConfig.Get() == 1]);
  playView.AddUIComponent(notePad1, Point(0, 0));

  UnderglowLight underglow1(underglowSize, notePadConfigs[activeConfig.Get() == 1].color);
  playView.AddUIComponent(underglow1, Point(-1, -1));

  NotePad notePad2(padSize, &notePadConfigs[activeConfig.Get() == 0]);
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

  NotePad notePad(Dimension(8, 4), &notePadConfigs[activeConfig]);
  colorSelector.AddUIComponent(notePad, Point(0, 0));

  UIButton rootColorSelectorBtn;
  rootColorSelectorBtn.SetName("Root Key Color");
  rootColorSelectorBtn.SetColorFunc([&]() -> Color { return notePadConfigs[activeConfig].rootColor; });
  rootColorSelectorBtn.SetSize(Dimension(2, 2));
  rootColorSelectorBtn.OnPress([&]() -> void { MatrixOS::UIUtility::ColorPicker(notePadConfigs[activeConfig].rootColor); });
  colorSelector.AddUIComponent(rootColorSelectorBtn, Point(1, 5));

  UIButton notePadColorSelectorBtn;
  notePadColorSelectorBtn.SetName("Note Pad Color");
  notePadColorSelectorBtn.SetColorFunc([&]() -> Color { return notePadConfigs[activeConfig].color; });
  notePadColorSelectorBtn.SetSize(Dimension(2, 2));
  notePadColorSelectorBtn.OnPress([&]() -> void { MatrixOS::UIUtility::ColorPicker(notePadConfigs[activeConfig].color); });
  colorSelector.AddUIComponent(notePadColorSelectorBtn, Point(5, 5));

  colorSelector.Start();
}

void Note::LayoutSelector() {
  UI layoutSelector("Layout Selector", Color(0xFFFF00), false);

  Color modeColor[3] = {
    Color(0xFFFF00),  // OCTAVE_LAYOUT - Lime Green
    Color(0x80FF00),  // OFFSET_LAYOUT - Blue
    Color(0x0080FF)   // PIANO_LAYOUT - Magenta
  };

  int32_t x_offset = notePadConfigs[activeConfig].x_offset;
  int32_t y_offset = notePadConfigs[activeConfig].y_offset;

  UIButton octaveModeBtn;
  octaveModeBtn.SetName("Octave Mode");
  octaveModeBtn.SetColorFunc([&]() -> Color { return modeColor[0].DimIfNot(notePadConfigs[activeConfig].mode == OCTAVE_LAYOUT); });
  octaveModeBtn.OnPress([&]() -> void { 
    if(notePadConfigs[activeConfig].mode != OCTAVE_LAYOUT)
    {
      notePadConfigs[activeConfig].mode = OCTAVE_LAYOUT; 
    }
  });
  layoutSelector.AddUIComponent(octaveModeBtn, Point(2, 0));

  UIButton offsetModeBtn;
  offsetModeBtn.SetName("Offset Mode");
  offsetModeBtn.SetColorFunc([&]() -> Color { return modeColor[1].DimIfNot(notePadConfigs[activeConfig].mode == OFFSET_LAYOUT); });
  offsetModeBtn.OnPress([&]() -> void { 
    if(notePadConfigs[activeConfig].mode != OFFSET_LAYOUT)
    {
      notePadConfigs[activeConfig].mode = OFFSET_LAYOUT;
      x_offset = 1;
      y_offset = 3;
    }
  });
  layoutSelector.AddUIComponent(offsetModeBtn, Point(3, 0));

  UIButton pianoModeBtn;
  pianoModeBtn.SetName("Piano Mode");
  pianoModeBtn.SetColorFunc([&]() -> Color { return modeColor[2].DimIfNot(notePadConfigs[activeConfig].mode == PIANO_LAYOUT); });
  pianoModeBtn.OnPress([&]() -> void { 
    if(notePadConfigs[activeConfig].mode != PIANO_LAYOUT)
    {
      notePadConfigs[activeConfig].mode = PIANO_LAYOUT;
    }
  });
  layoutSelector.AddUIComponent(pianoModeBtn, Point(4, 0));

  // Offset Mode
  UISelector yOffsetInput;
  yOffsetInput.SetDimension(Dimension(1, 8));
  yOffsetInput.SetName("Y Offset");
  yOffsetInput.SetColor(Color(0x00FF00));
  yOffsetInput.SetValuePointer((uint16_t*)&y_offset);
  yOffsetInput.SetLitMode(UISelectorLitMode::LIT_LESS_EQUAL_THAN);
  yOffsetInput.SetDirection(UISelectorDirection::UP_THEN_RIGHT);
  yOffsetInput.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(yOffsetInput, Point(0, 0));

  UI4pxNumber yOffsetDisplay;
  yOffsetDisplay.SetColor(Color(0x00FF00));
  yOffsetDisplay.SetDigits(1);
  yOffsetDisplay.SetValuePointer((int32_t*)&y_offset);
  yOffsetDisplay.SetAlternativeColor(Color(0x00FF00));
  yOffsetDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(yOffsetDisplay, Point(2, 2));

  UISelector xOffsetInput;
  xOffsetInput.SetDimension(Dimension(8, 1));
  xOffsetInput.SetName("X Offset");
  xOffsetInput.SetColor(Color(0xFF0000));
  xOffsetInput.SetValuePointer((uint16_t*)&x_offset);
  xOffsetInput.SetLitMode(UISelectorLitMode::LIT_LESS_EQUAL_THAN);
  xOffsetInput.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(xOffsetInput, Point(0, 7));

  UI4pxNumber xOffsetDisplay;
  xOffsetDisplay.SetColor(Color(0xFF0000));
  xOffsetDisplay.SetDigits(1);
  xOffsetDisplay.SetValuePointer((int32_t*)&x_offset);
  xOffsetDisplay.SetAlternativeColor(Color(0xFF0000));
  xOffsetDisplay.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(xOffsetDisplay, Point(5, 2));

  // Show Off Scale Notes Toggle (only for Octave and Offset modes)
  UIButton includeOutScaleNotesToggle;
  includeOutScaleNotesToggle.SetName("Include Out of Scale Notes");
  includeOutScaleNotesToggle.SetColorFunc([&]() -> Color { return Color(0xFFFFFF).DimIfNot(notePadConfigs[activeConfig].includeOutScaleNotes); });
  includeOutScaleNotesToggle.OnPress([&]() -> void { notePadConfigs[activeConfig].includeOutScaleNotes = !notePadConfigs[activeConfig].includeOutScaleNotes; });
  includeOutScaleNotesToggle.SetEnableFunc([&]() -> bool { return notePadConfigs[activeConfig].mode == OCTAVE_LAYOUT || notePadConfigs[activeConfig].mode == OFFSET_LAYOUT; });
  layoutSelector.AddUIComponent(includeOutScaleNotesToggle, Point(0, 7));

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
  numDisplay.SetAlternativeColor(notePadConfigs[activeConfig].rootColor);
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