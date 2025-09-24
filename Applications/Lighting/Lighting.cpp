#include "Lighting.h"
#include "BPMTapper.h"

void Lighting::Setup(const vector<string>& args) {
  start_time = MatrixOS::SYS::Millis();
  Update();
}

void Lighting::Loop()
{
  struct KeyEvent keyEvent;
  while (MatrixOS::KeyPad::Get(&keyEvent))
  { KeyEventHandler(keyEvent); }

  if(renderTimer.Tick(1000/Device::LED::fps))
  {
    Update();
  }
}

void Lighting::Update()
{
  switch(mode)
  {
   case RGB:
    {
      uint16_t period = 60000 / rgb_effect_bpm.Get(); // Convert BPM to period in ms
      Color color = ApplyColorEffect(this->color.Get(), rgb_effect, period, start_time);
      Render(color);
      break;
    }
    case Temperature:
    {
      uint16_t period = 60000 / temperature_effect_bpm.Get(); // Convert BPM to period in ms
      Color color = ApplyColorEffect(temperature_color, temperature_effect, period, start_time);
      Render(color);
      break;
    }
    // case Animation:
    // {
    //   Color color = GetAnimationColor(animation.Get(), start_time);
    //   Render(color);
    //   break;
    // }
    // case Gradient:
    //   RenderGradient();
    // break; 
  }
}

Color Lighting::ApplyColorEffect(Color color, ColorEffectMode effect, uint16_t period, uint16_t start_time)
{
  switch(effect)
  {
    case Static:
      break;  
    case Breath:
      color = ColorEffects::ColorBreath(color, period, start_time);
      break;
    case Strobe:
      color = ColorEffects::ColorStrobe(color, period, start_time);
      break;
    case Saw:
      color = ColorEffects::ColorSaw(color, period, start_time);
      break;
  }
  return color;
}

Color Lighting::GetAnimationColor(Animations animation, uint16_t start_time)
{
  Color color = Color(0xFFFFFF);
  switch(animation)
  {
    case PoliceCar:
      break;
    case Breathing:
      break;
  }

  return color;
}

void Lighting::Render(Color color)
{
  // TODO: Get chunks, check if should render, render

  MatrixOS::LED::Fill(color);
  MatrixOS::LED::Update();
}

void Lighting::RenderGradient()
{
}

void Lighting::KeyEventHandler(KeyEvent& keyEvent) {
  if (keyEvent.ID() == FUNCTION_KEY)
  {
    if (keyEvent.State() == PRESSED)
    {
      Settings();
    }
  }
}

void Lighting::Settings() {
  UI settingsUI("Settings", Color(0x00FFFF), true);

  start_time = MatrixOS::SYS::Millis();

  
// Lighting Mode Selector & chunk selector
  // Selector
  UIButton rgbModeBtn;
  rgbModeBtn.SetName("RGB Mode");
  rgbModeBtn.SetColorFunc([&]() -> Color { return Color(0xFF00FF).DimIfNot(mode == RGB); });
  rgbModeBtn.OnPress([&]() -> void { mode = RGB; });
  settingsUI.AddUIComponent(rgbModeBtn, Point(3, 0));

  UIButton temperatureModeBtn;
  temperatureModeBtn.SetName("Temperature Mode");
  temperatureModeBtn.SetColorFunc([&]() -> Color { return Color(0xFFFFFF).DimIfNot(mode == Temperature); });
  temperatureModeBtn.OnPress([&]() -> void { mode = Temperature; });
  settingsUI.AddUIComponent(temperatureModeBtn, Point(4, 0));

  // UIButton animationModeBtn;
  // animationModeBtn.SetName("Animation Mode");
  // animationModeBtn.SetColorFunc([&]() -> Color { return Color(0xFFFF00).DimIfNot(mode == Animation); });
  // animationModeBtn.OnPress([&]() -> void { mode = Animation; });
  // settingsUI.AddUIComponent(animationModeBtn, Point(4, 0));

  // UIButton gradientModeBtn;
  // gradientModeBtn.SetName("Gradient Mode");
  // gradientModeBtn.SetColorFunc([&]() -> Color { return Color(0x00FFFF).DimIfNot(mode == Gradient); });
  // gradientModeBtn.OnPress([&]() -> void { mode = Gradient; });
  // settingsUI.AddUIComponent(gradientModeBtn, Point(5, 0));

  // Chunk Selector
  // TODO

  // RGB Mode
  // Color
  UIButton colorBtn;
  colorBtn.SetName("Color");
  colorBtn.SetColorFunc([&]() -> Color { return color; });
  colorBtn.SetSize(Dimension(8,2));
  colorBtn.OnPress([&]() -> void { if(MatrixOS::UIUtility::ColorPicker(color.Get())) { color.Save(); } });
  colorBtn.SetEnableFunc([&]() -> bool { return mode == RGB; });
  settingsUI.AddUIComponent(colorBtn, Point(0, 6));

  // Mode & Speed
  UIButton rgbEffectBtn;
  rgbEffectBtn.SetName("Effect & Speed");
  rgbEffectBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, rgb_effect, 60000 / rgb_effect_bpm.Get(), start_time); });
  rgbEffectBtn.OnPress([&]() -> void { EffectModeAndSpeedMenu(RGB); });
  rgbEffectBtn.SetEnableFunc([&]() -> bool { return mode == RGB; });
  rgbEffectBtn.SetSize(Dimension(1, 2));
  settingsUI.AddUIComponent(rgbEffectBtn, Point(0, 3));

  // Temperature Mode
  // Temperature
  UIButton temperatureBtn;
  temperatureBtn.SetName("Temperature");
  temperatureBtn.SetColorFunc([&]() -> Color { return temperature_color; });
  temperatureBtn.SetSize(Dimension(8,2));
  temperatureBtn.OnPress([&]() -> void { if(MatrixOS::UIUtility::TemperatureColorPicker(temperature_color.Get())) { temperature_color.Save(); } });
  temperatureBtn.SetEnableFunc([&]() -> bool { return mode == Temperature; });
  settingsUI.AddUIComponent(temperatureBtn, Point(0, 6));

  // Mode & Speed
  UIButton temperatureEffectBtn;
  temperatureEffectBtn.SetName("Effect & Speed");
  temperatureEffectBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(temperature_color, temperature_effect, 60000 / temperature_effect_bpm.Get(), start_time); });
  temperatureEffectBtn.OnPress([&]() -> void { EffectModeAndSpeedMenu(Temperature); });
  temperatureEffectBtn.SetEnableFunc([&]() -> bool { return mode == Temperature; });
  temperatureEffectBtn.SetSize(Dimension(1, 2));
  settingsUI.AddUIComponent(temperatureEffectBtn, Point(0, 3));

  // Animation
    // Animation
    // Speed

  // Gradient
    // Gradient Selector
  
  // Second, set the key event handler to match the intended behavior
  settingsUI.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    // If function key is hold down. Exit the application
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == HOLD)
      {
        Exit();  // Exit the application.
      }
      else if (keyEvent->info.state == RELEASED)
      {
        settingsUI.Exit();  // Exit the UI
      }

      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;  // Nothing happened. Let the UI handle the key event
  });
  
  // The UI object is now fully set up. Let the UI runtime to start and take over.
  settingsUI.Start();

  // Reset the start time to help user sync.
  start_time = MatrixOS::SYS::Millis();
}

void Lighting::EffectModeAndSpeedMenu(LightingMode mode)
{
  Color color;

  if(mode == RGB)
  {
    color = this->color.Get();
  }
  else if(mode == Temperature)
  {
    color = temperature_color.Get();
  }

  UI effectUI("Effect Mode", Color(color), true);

  ColorEffectMode effectMode = (mode == RGB) ? rgb_effect : temperature_effect;
  int32_t bpm = (mode == RGB) ? rgb_effect_bpm : temperature_effect_bpm;

  // uint16_t period = 60000 / bpm; // Convert BPM to period in ms

  start_time = MatrixOS::SYS::Millis();

  UIButton staticBtn;
  staticBtn.SetName("Static");
  staticBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, Static, 60000 / bpm, start_time).DimIfNot(effectMode == Static); });
  staticBtn.OnPress([&]() -> void { effectMode = Static; });
  effectUI.AddUIComponent(staticBtn, Point(2, 0));

  UIButton breathBtn;
  breathBtn.SetName("Breathing");
  breathBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, Breath, 60000 / bpm, start_time).DimIfNot(effectMode == Breath); });
  breathBtn.OnPress([&]() -> void { effectMode = Breath; });
  effectUI.AddUIComponent(breathBtn, Point(3, 0));

  UIButton strobeBtn;
  strobeBtn.SetName("Strobe");
  strobeBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, Strobe, 60000 / bpm, start_time).DimIfNot(effectMode == Strobe); });
  strobeBtn.OnPress([&]() -> void { effectMode = Strobe; });
  effectUI.AddUIComponent(strobeBtn, Point(4, 0));

  UIButton sawBtn;
  sawBtn.SetName("Saw");
  sawBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, Saw, 60000 / bpm, start_time).DimIfNot(effectMode == Saw); });
  sawBtn.OnPress([&]() -> void { effectMode = Saw; });
  effectUI.AddUIComponent(sawBtn, Point(5, 0));

  UI4pxNumber bpmDisplay;
  bpmDisplay.SetName("BPM");
  bpmDisplay.SetColorFunc([&](uint16_t digit) -> Color { return digit % 2 ? Color(0xFFFFFF) : Color(0xFF00FF); });
  bpmDisplay.SetDigits(3);
  bpmDisplay.SetValuePointer((int32_t*)&bpm);
  effectUI.AddUIComponent(bpmDisplay, Point(-1, 2));

  UIBPMTapper bpmTapper;
  bpmTapper.SetColor(Color(0xFF00FF));
  bpmTapper.SetSize(Dimension(8,4));
  bpmTapper.OnChange([&](uint16_t new_bpm) -> void { bpm = new_bpm; });
  effectUI.AddUIComponent(bpmTapper, Point(0, 2));


  int32_t modifier[8] = {-50, -20, -5, -1, 1, 5, 20, 50};
  uint8_t gradient[8] = {255, 127, 64, 32, 32, 64, 127, 255};
  UINumberModifier bpmModifier;
  bpmModifier.SetColor(Color(0xFF00FF));
  bpmModifier.SetLength(8);
  bpmModifier.SetValuePointer((int32_t*)&bpm);
  bpmModifier.SetModifiers(modifier);
  bpmModifier.SetControlGradient(gradient);
  bpmModifier.SetLowerLimit(10);
  bpmModifier.SetUpperLimit(299);
  effectUI.AddUIComponent(bpmModifier, Point(0, 7));

  effectUI.Start();

  // Post exit save variable
  if(mode == RGB)
  {
    rgb_effect = effectMode;
    rgb_effect_bpm = bpm;
  }
  else if(mode == Temperature)
  {
    temperature_effect = effectMode;
    temperature_effect_bpm = bpm;
  }
}