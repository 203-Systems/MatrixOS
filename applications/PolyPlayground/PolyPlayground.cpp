#include "PolyPlayground.h"
#include "PolyOctaveShifter.h"
#include "applications/BrightnessControl/BrightnessControl.h"
#include "../Note/ScaleVisualizer.h"

void PolyPlayground::Setup() {
  // Set up / Load configs --------------------------------------------------------------------------

  // Load From NVS
  if (nvsVersion == (uint32_t)POLY_PLAYGROUND_APP_VERSION)
  { MatrixOS::NVS::GetVariable(POLY_CONFIGS_HASH, &polyPadConfig, sizeof(polyPadConfig)); }
  else
  { 
    MatrixOS::NVS::SetVariable(POLY_CONFIGS_HASH, &polyPadConfig, sizeof(polyPadConfig));
    nvsVersion = POLY_PLAYGROUND_APP_VERSION;
  }

  // Set up the Action Menu UI ---------------------------------------------------------------------
  UI actionMenu("Action Menu", PolyPlayground::info.color, false);

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

  UIButton rootSelectorBtn;
  rootSelectorBtn.SetName("Root Selector");
  rootSelectorBtn.SetColor(Color(0xFF0090));
  rootSelectorBtn.OnPress([&]() -> void { RootSelector(); });
  actionMenu.AddUIComponent(rootSelectorBtn, Point(7, 2));


  UIButton channelSelectorBtn;
  channelSelectorBtn.SetName("Channel Selector");
  channelSelectorBtn.SetColor(Color(0x60FF00));
  channelSelectorBtn.OnPress([&]() -> void { ChannelSelector(); });
  actionMenu.AddUIComponent(channelSelectorBtn, Point(7, 4));

  UIButton velocitySensitiveToggle;
  velocitySensitiveToggle.SetName("Velocity Sensitive");
  velocitySensitiveToggle.SetColorFunc([&]() -> Color { return  Color(0x00FFB0).DimIfNot(polyPadConfig.velocitySensitive); });
  velocitySensitiveToggle.OnPress([&]() -> void { polyPadConfig.velocitySensitive = !polyPadConfig.velocitySensitive; });
  velocitySensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll(velocitySensitiveToggle.GetName() + " " + (polyPadConfig.velocitySensitive ? "On" : "Off"), velocitySensitiveToggle.GetColor()); });
  velocitySensitiveToggle.SetEnabled(Device::KeyPad::velocity_sensitivity);
  actionMenu.AddUIComponent(velocitySensitiveToggle, Point(6, 7));

  PolyOctaveShifter octaveShifter(8, &polyPadConfig);
  actionMenu.AddUIComponent(octaveShifter, Point(0, 0));

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
        MatrixOS::NVS::SetVariable(POLY_CONFIGS_HASH, &polyPadConfig, sizeof(polyPadConfig));
        PolyView();
      }
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });
  actionMenu.AllowExit(false);
  actionMenu.SetSetupFunc([&]() -> void {PolyView();});
  actionMenu.Start();

  Exit(); // This should never be reached
}

void PolyPlayground::PolyView() {
  UI PolyView("Poly View", PolyPlayground::info.color, false);

  PolyPad polyPad(Dimension(8, 8), &polyPadConfig);
  PolyView.AddUIComponent(polyPad, Point(0, 0));


  PolyView.Start();
}

void PolyPlayground::ChannelSelector() {
  UI channelSelector("Channel Selector", Color(0x60FF00), false);

  int32_t offsettedChannel = polyPadConfig.channel + 1;
  UI4pxNumber numDisplay(Color(0x60FF00), 2, &offsettedChannel, Color(0xFFFFFF), 1);
  channelSelector.AddUIComponent(numDisplay, Point(1, 0));

  UISelector channelInput(Dimension(8, 2), "Channel", Color(0x60FF00), 16, (uint16_t*)&polyPadConfig.channel,
                          [&](uint16_t val) -> void { offsettedChannel = val + 1; });

  channelSelector.AddUIComponent(channelInput, Point(0, 6));

  channelSelector.Start();
}

void PolyPlayground::RootSelector() {
  UI scaleSelector("Root Selector", Color(0xFF0090), false);
  uint16_t scale = 0;
  ScaleVisualizer scaleVisualizer(&polyPadConfig.rootKey, &scale, Color(0x8000FF), Color(0xFF00FF));
  scaleSelector.AddUIComponent(scaleVisualizer, Point(0, 0));

  scaleSelector.Start();
}
