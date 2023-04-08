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
  if (nvsVersion == NOTE_APP_VERSION)
  { MatrixOS::NVS::GetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs)); }
  else
  { MatrixOS::NVS::SetVariable(NOTE_CONFIGS_HASH, notePadConfigs, sizeof(notePadConfigs)); }

  activeConfig.Get(); //Load it first

  // Set up the Action Menu UI ---------------------------------------------------------------------
  UI actionMenu("Action Menu", Color(0x00FFFF));

  UIButtonLarge brightnessBtn(
      "Brightness", Color(0xFFFFFF), Dimension(2, 2), [&]() -> void { MatrixOS::SYS::NextBrightness(); }, [&]() -> void { BrightnessControl().Start(); });
  actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButtonLarge rotateUpBtn("This does nothing", Color(0x00FF00), Dimension(2, 1), [&]() -> void {});
  actionMenu.AddUIComponent(rotateUpBtn, Point(3, 2));

  UIButtonLarge rotatRightBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2), [&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  actionMenu.AddUIComponent(rotatRightBtn, Point(5, 3));

  UIButtonLarge rotateDownBtn("Rotate to this side", Color(0x00FF00), Dimension(2, 1), [&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  actionMenu.AddUIComponent(rotateDownBtn, Point(3, 5));

  UIButtonLarge rotateLeftBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2), [&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  actionMenu.AddUIComponent(rotateLeftBtn, Point(2, 3));

  // Note Pad Control
  UIButton scaleSelectorBtn("Scale Selector", Color(0xFF0090), [&]() -> void { ScaleSelector(); });
  actionMenu.AddUIComponent(scaleSelectorBtn, Point(7, 2));

  UIButtonDimmable enforceScaleToggle(
      "Enforce Scale", Color(0xff5000), [&]() -> bool { return notePadConfigs[activeConfig].enfourceScale; },
      [&]() -> void { notePadConfigs[activeConfig].enfourceScale = !notePadConfigs[activeConfig].enfourceScale; });
  actionMenu.AddUIComponent(enforceScaleToggle, Point(7, 3));

  UIButton overlapSelectorBtn("Overlap Selector", Color(0xFFFF00), [&]() -> void { OverlapSelector(); });
  actionMenu.AddUIComponent(overlapSelectorBtn, Point(7, 4));

  UIButton channelSelectorBtn("Channel Selector", Color(0x60FF00), [&]() -> void { ChannelSelector(); });
  actionMenu.AddUIComponent(channelSelectorBtn, Point(7, 5));

  UIButtonDimmable velocitySensitiveToggle(
      "Velocity Sensitive", Color(0x00FFB0), [&]() -> bool { return notePadConfigs[activeConfig].velocitySensitive; },
      [&]() -> void { notePadConfigs[activeConfig].velocitySensitive = !notePadConfigs[activeConfig].velocitySensitive; });
  actionMenu.AddUIComponent(velocitySensitiveToggle, Point(6, 7));

  OctaveShifter octaveShifter(8, notePadConfigs, &activeConfig.value);
  actionMenu.AddUIComponent(octaveShifter, Point(0, 0));

  // Split View
  UIButtonDimmable splitViewToggle(
      "Split View", Color(0xFFFFFF), [&]() -> bool { return splitView; }, [&]() -> void { splitView = !splitView; });
  actionMenu.AddUIComponent(splitViewToggle, Point(1, 0));

  UIButtonWithColorFunc notepad1SelectBtn(
      "Note Pad 1", [&]() -> Color { return notePadConfigs[0].color.ToLowBrightness(activeConfig.Get() == 0); }, [&]() -> void { activeConfig = 0; });
  actionMenu.AddUIComponent(notepad1SelectBtn, Point(3, 0));

  UIButtonWithColorFunc notepad2SelectBtn(
      "Note Pad 2", [&]() -> Color { return notePadConfigs[1].color.ToLowBrightness(activeConfig.Get() == 1); }, [&]() -> void { activeConfig = 1; });
  actionMenu.AddUIComponent(notepad2SelectBtn, Point(4, 0));

  UIButtonWithColorFunc notepadColorBtn(
      "Note Pad Color", [&]() -> Color { return notePadConfigs[activeConfig].color; }, [&]() -> void { ColorSelector(); });
  actionMenu.AddUIComponent(notepadColorBtn, Point(7, 0));

  // Other Controls
  UIButton systemSettingBtn("System Setting", Color(0xFFFFFF), [&]() -> void { MatrixOS::SYS::OpenSetting(); });
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
  actionMenu.Start();

  Exit();
}

void Note::PlayView() {
  UI playView("Note Play View");

  NotePad notePad1(Dimension(splitView ? 4 : 8, 8), &notePadConfigs[!splitView && activeConfig.Get() == 1]);
  playView.AddUIComponent(notePad1, Point(0, 0));

  NotePad notePad2(Dimension(4, 8), &notePadConfigs[1]);
  if (splitView)
  { playView.AddUIComponent(notePad2, Point(4, 0)); }

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

  UIButtonLargeWithColorFunc rootColorSelectorBtn(
      "Root Key Color", [&]() -> Color { return notePadConfigs[activeConfig].rootColor; }, Dimension(2, 2),
      [&]() -> void { MatrixOS::UIInterface::ColorPicker(notePadConfigs[activeConfig].rootColor); });
  colorSelector.AddUIComponent(rootColorSelectorBtn, Point(1, 5));

  UIButtonLargeWithColorFunc notePadColorSelectorBtn(
      "Note Pad Color", [&]() -> Color { return notePadConfigs[activeConfig].color; }, Dimension(2, 2),
      [&]() -> void { MatrixOS::UIInterface::ColorPicker(notePadConfigs[activeConfig].color); });
  colorSelector.AddUIComponent(notePadColorSelectorBtn, Point(5, 5));

  colorSelector.Start();
}

void Note::OverlapSelector() {
  UI overlapSelector("Overlap Selector", Color(0xFFFF00));

  UI4pxNumber numDisplay(Color(0xFFFF00), 1, (int32_t*)&notePadConfigs[activeConfig].overlap, notePadConfigs[activeConfig].rootColor);
  overlapSelector.AddUIComponent(numDisplay, Point(5, 0));

  UISelector overlapInput(Dimension(7, 1), "Overlap", Color(0xFFFF00), 7, (uint16_t*)&notePadConfigs[activeConfig].overlap);
  overlapSelector.AddUIComponent(overlapInput, Point(0, 7));

  UIButtonDimmable alignRootToggle(
      "Aligh Root Key", Color(0xFFFFFF), [&]() -> bool { return notePadConfigs[activeConfig].alignRoot; },
      [&]() -> void { notePadConfigs[activeConfig].alignRoot = !notePadConfigs[activeConfig].alignRoot; });
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