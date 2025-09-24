#include "PolyPlayground.h"
#include "PolyOctaveShifter.h"
#include "../Note/ScaleVisualizer.h"

void PolyPlayground::Setup(const vector<string>& args) {
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

  UIButton forceSensitiveToggle;
  forceSensitiveToggle.SetName("Velocity Sensitive");
  forceSensitiveToggle.SetColorFunc([&]() -> Color { return  Color(0x00FFB0).DimIfNot(polyPadConfig.forceSensitive); });
  forceSensitiveToggle.OnPress([&]() -> void { polyPadConfig.forceSensitive = !polyPadConfig.forceSensitive; });
  forceSensitiveToggle.OnHold([&]() -> void { MatrixOS::UIUtility::TextScroll(forceSensitiveToggle.GetName() + " " + (polyPadConfig.forceSensitive ? "On" : "Off"), forceSensitiveToggle.GetColor()); });
  forceSensitiveToggle.SetEnabled(Device::KeyPad::velocity_sensitivity);
  actionMenu.AddUIComponent(forceSensitiveToggle, Point(6, 7));

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
  channelInput.SetValuePointer((uint16_t*)&polyPadConfig.channel);
  channelInput.OnChange([&](uint16_t val) -> void { offsettedChannel = val + 1; });

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
