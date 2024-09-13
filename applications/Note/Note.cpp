#include "Note.h"
#include "OctaveShifter.h"
#include "ScaleVisualizer.h"

#include "applications/BrightnessControl/BrightnessControl.h"

void Note::Setup() {
  // Set up / Load configs --------------------------------------------------------------------------

  // Default Values
  notePadConfigs[1].color = Color(0xFF00FF);
  notePadConfigs[1].rootColor = Color(0x8800FF);

  // Load From NVS
  if (nvsVersion == (uint32_t)NOTE_APP_VERSION)
  { MatrixOS::NVS::GetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs)); }
  else
  { MatrixOS::NVS::SetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs)); }

  activeConfig.Get(); //Load it first

  // Set up the Action Menu UI ---------------------------------------------------------------------
  UI actionMenu("Action Menu", Color(0x00FFFF));

  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([&]() -> void { MatrixOS::LED::NextBrightness(); });
  brightnessBtn.OnHold([&]() -> void { BrightnessControl().Start(); });
  actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButton rotateUpBtn;
  rotateUpBtn.SetName("This does nothing");
  rotateUpBtn.SetColor(Color(0x00FF00));
  rotateUpBtn.SetSize(Dimension(2, 1));
  rotateUpBtn.OnPress([&]() -> void {});
  actionMenu.AddUIComponent(rotateUpBtn, Point(3, 2));

  UIButton rotatRightBtn;
  rotatRightBtn.SetName("Rotate to this side");
  rotatRightBtn.SetColor(Color(0x00FF00));
  rotatRightBtn.SetSize(Dimension(1, 2));
  rotatRightBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  actionMenu.AddUIComponent(rotatRightBtn, Point(5, 3));

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

  // Note Pad Control
  UIButton scaleSelectorBtn;
  scaleSelectorBtn.SetName("Scale Selector");
  scaleSelectorBtn.SetColor(Color(0xFF0090));
  scaleSelectorBtn.OnPress([&]() -> void { ScaleSelector(); });
  actionMenu.AddUIComponent(scaleSelectorBtn, Point(7, 2));

  UIToggle enforceScaleToggle;
  enforceScaleToggle.SetName("Enforce Scale");
  enforceScaleToggle.SetColor(Color(0xff5000));
  actionMenu.AddUIComponent(enforceScaleToggle, Point(7, 3));

  UIButton overlapSelectorBtn;
  overlapSelectorBtn.SetName("Overlap Selector");
  overlapSelectorBtn.SetColor(Color(0xFFFF00));
  overlapSelectorBtn.OnPress([&]() -> void { OverlapSelector(); });
  actionMenu.AddUIComponent(overlapSelectorBtn, Point(7, 4));

  UIButton channelSelectorBtn;
  channelSelectorBtn.SetName("Channel Selector");
  channelSelectorBtn.SetColor(Color(0x60FF00));
  channelSelectorBtn.OnPress([&]() -> void { ChannelSelector(); });
  actionMenu.AddUIComponent(channelSelectorBtn, Point(7, 5));

  UIToggle velocitySensitiveToggle;
  velocitySensitiveToggle.SetName("Velocity Sensitive");
  velocitySensitiveToggle.SetColor(Color(0x00FFB0));
  velocitySensitiveToggle.SetEnabled(Device::KeyPad::velocity_sensitivity);
  actionMenu.AddUIComponent(velocitySensitiveToggle, Point(6, 7));

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
      case SINGLE_VIEW: MatrixOS::UIInterface::TextScroll("Single View", Color(0xFFFFFF)); break;
      case VERT_SPLIT: MatrixOS::UIInterface::TextScroll("Vertical Split", Color(0x00FFFF)); break;
      case HORIZ_SPLIT: MatrixOS::UIInterface::TextScroll("Horizontal Split", Color(0xFF00FF)); break;
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
        PlayView();
        MatrixOS::NVS::SetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs));
      }
      return true;  // Block UI from to do anything with FN, basiclly this function control the life cycle of the UI
    }
    return false;
  });
  actionMenu.AllowExit(false);
  actionMenu.SetSetupFunc([&]() -> void {PlayView();});
  actionMenu.Start();

  Exit();
}

void Note::PlayView() {
  UI playView("Note Play View");

  Dimension padSize;

  switch (splitView)
  {
    case SINGLE_VIEW:
      padSize = Dimension(8, 8);
      break;
    case VERT_SPLIT:
      padSize = Dimension(4, 8);
      break;
    case HORIZ_SPLIT:
      padSize = Dimension(8, 4);
      break;
  }
  

  NotePad notePad1(padSize, &notePadConfigs[!splitView && activeConfig.Get() == 1]);
  playView.AddUIComponent(notePad1, Point(0, 0));

  NotePad notePad2(padSize, &notePadConfigs[1]);
  if (splitView == VERT_SPLIT) { playView.AddUIComponent(notePad2, Point(4, 0)); }
  else if (splitView == HORIZ_SPLIT) { playView.AddUIComponent(notePad2, Point(0, 4)); }

  playView.Start();
}

void Note::ScaleSelector() {
  UI scaleSelector("Scale Selector", Color(0xFF0090));

  ScaleVisualizer scaleVisualizer(&notePadConfigs[activeConfig].rootKey, &notePadConfigs[activeConfig].scale, notePadConfigs[activeConfig].color,
                                  notePadConfigs[activeConfig].rootColor);
  scaleSelector.AddUIComponent(scaleVisualizer, Point(0, 0));

  UIItemSelector scaleSelectorBar(Dimension(8, 4), Color(0xFF0090), &notePadConfigs[activeConfig].scale, 32, scales, scale_names);
  scaleSelector.AddUIComponent(scaleSelectorBar, Point(0, 4));

  scaleSelector.Start();
}

void Note::ColorSelector() {
  UI colorSelector("Color Selector", notePadConfigs[activeConfig].color);

  NotePad notePad(Dimension(8, 4), &notePadConfigs[activeConfig]);
  colorSelector.AddUIComponent(notePad, Point(0, 0));

  UIButton rootColorSelectorBtn;
  rootColorSelectorBtn.SetName("Root Key Color");
  rootColorSelectorBtn.SetColorFunc([&]() -> Color { return notePadConfigs[activeConfig].rootColor; });
  rootColorSelectorBtn.SetSize(Dimension(2, 2));
  rootColorSelectorBtn.OnPress([&]() -> void { MatrixOS::UIInterface::ColorPicker(notePadConfigs[activeConfig].rootColor); });
  colorSelector.AddUIComponent(rootColorSelectorBtn, Point(1, 5));

  UIButton notePadColorSelectorBtn;
  notePadColorSelectorBtn.SetName("Note Pad Color");
  notePadColorSelectorBtn.SetColorFunc([&]() -> Color { return notePadConfigs[activeConfig].color; });
  notePadColorSelectorBtn.SetSize(Dimension(2, 2));
  notePadColorSelectorBtn.OnPress([&]() -> void { MatrixOS::UIInterface::ColorPicker(notePadConfigs[activeConfig].color); });
  colorSelector.AddUIComponent(notePadColorSelectorBtn, Point(5, 5));

  colorSelector.Start();
}

void Note::OverlapSelector() {
  UI overlapSelector("Overlap Selector", Color(0xFFFF00));

  UI4pxNumber numDisplay(Color(0xFFFF00), 1, (int32_t*)&notePadConfigs[activeConfig].overlap, notePadConfigs[activeConfig].rootColor);
  overlapSelector.AddUIComponent(numDisplay, Point(5, 0));

  UISelector overlapInput(Dimension(7, 1), "Overlap", Color(0xFFFF00), 7, (uint16_t*)&notePadConfigs[activeConfig].overlap);
  overlapSelector.AddUIComponent(overlapInput, Point(0, 7));

  UIButton alignRootToggle;
  alignRootToggle.SetName("Align Root Key");
  alignRootToggle.SetColorFunc([&]() -> Color { return Color(0xFFFFFF).DimIfNot(notePadConfigs[activeConfig].alignRoot); });
  alignRootToggle.OnPress([&]() -> void { notePadConfigs[activeConfig].alignRoot = !notePadConfigs[activeConfig].alignRoot; });
  overlapSelector.AddUIComponent(alignRootToggle, Point(7, 7));

  overlapSelector.Start();
}

void Note::ChannelSelector() {
  UI channelSelector("Channel Selector", Color(0x60FF00));

  int32_t offsettedChannel = notePadConfigs[activeConfig].channel + 1;
  UI4pxNumber numDisplay(Color(0x60FF00), 2, &offsettedChannel, notePadConfigs[activeConfig].rootColor, 1);
  channelSelector.AddUIComponent(numDisplay, Point(1, 0));

  UISelector channelInput(Dimension(8, 2), "Channel", Color(0x60FF00), 16, (uint16_t*)&notePadConfigs[activeConfig].channel,
                          [&](uint16_t val) -> void { offsettedChannel = val + 1; });

  channelSelector.AddUIComponent(channelInput, Point(0, 6));

  channelSelector.Start();
}