#include "Lighting.h"

void Lighting::Setup() {
  start_time = MatrixOS::SYS::Millis();
  Update();
}

void Lighting::Loop()
{
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }

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
      Color color = ApplyColorEffect(this->color.Get(), rgb_effect, rgb_effect_period.Get(), start_time);
      Render(color);
      break;
    }
    case Temperature:
    {
      Color color = ApplyColorEffect(temperature_color, temperature_effect, temperature_effect_period.Get(), start_time); 
      Render(color);
      break;
    }
    case Animation:
    {
      Color color = GetAnimationColor(animation.Get(), start_time);
      Render(color);
      break;
    }
    case Gradient:
      RenderGradient();
    break; 
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

void Lighting::KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo) {
  if (keyID == FUNCTION_KEY)
  {
    if (keyInfo->state == PRESSED)
    {
      Settings();
    }
  }
}

void Lighting::Settings() {
  UI settingsUI("Settings", Color(0x00FFFF), true);

  start_time = MatrixOS::SYS::Millis();

  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([&]() -> void { MatrixOS::LED::NextBrightness(); });
  brightnessBtn.OnHold([&]() -> void { BrightnessControl().Start(); });
  settingsUI.AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButton rotateUpBtn;
  rotateUpBtn.SetName("Rotate Up");
  rotateUpBtn.SetColor(Color(0x00FF00));
  rotateUpBtn.SetSize(Dimension(2, 1));
  rotateUpBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(UP); });
  settingsUI.AddUIComponent(rotateUpBtn, Point(3, 2));

  UIButton rotateRightBtn;
  rotateRightBtn.SetName("Rotate Right");
  rotateRightBtn.SetColor(Color(0x00FF00));
  rotateRightBtn.SetSize(Dimension(1, 2));
  rotateRightBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  settingsUI.AddUIComponent(rotateRightBtn, Point(5, 3));

  UIButton rotateDownBtn;
  rotateDownBtn.SetName("Rotate Down");
  rotateDownBtn.SetColor(Color(0x00FF00));
  rotateDownBtn.SetSize(Dimension(2, 1));
  rotateDownBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  settingsUI.AddUIComponent(rotateDownBtn, Point(3, 5));

  UIButton rotateLeftBtn;
  rotateLeftBtn.SetName("Rotate Left");
  rotateLeftBtn.SetColor(Color(0x00FF00));
  rotateLeftBtn.SetSize(Dimension(1, 2));
  rotateLeftBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  settingsUI.AddUIComponent(rotateLeftBtn, Point(2, 3));

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

  // RGB
    // Color
    UIButton colorBtn;
    colorBtn.SetName("Color");
    colorBtn.SetColorFunc([&]() -> Color { return color; });
    colorBtn.SetSize(Dimension(8,2));
    colorBtn.OnPress([&]() -> void { if(MatrixOS::UIUtility::ColorPicker(color.Get())) { color.Save(); } });
    colorBtn.ShouldEnable([&]() -> bool { return mode == RGB; });
    settingsUI.AddUIComponent(colorBtn, Point(0, 6));

    // Mode & Speed
    UIButton rgbEffectBtn;
    rgbEffectBtn.SetName("Effect & Speed");
    rgbEffectBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, rgb_effect, rgb_effect_period.Get(), start_time); });
    rgbEffectBtn.OnPress([&]() -> void { EffectModeAndSpeedMenu(RGB); });
    rgbEffectBtn.ShouldEnable([&]() -> bool { return mode == RGB; });
    rgbEffectBtn.SetSize(Dimension(1, 2));
    settingsUI.AddUIComponent(rgbEffectBtn, Point(0, 3));

  // Temperature
    // Temperature
      UIButton temperatureBtn;
      temperatureBtn.SetName("Temperature");
      temperatureBtn.SetColorFunc([&]() -> Color { return temperature_color; });
      temperatureBtn.SetSize(Dimension(8,2));
      temperatureBtn.OnPress([&]() -> void { if(MatrixOS::UIUtility::TemperatureColorPicker(temperature_color.Get())) { temperature_color.Save(); } });
      temperatureBtn.ShouldEnable([&]() -> bool { return mode == Temperature; });
      settingsUI.AddUIComponent(temperatureBtn, Point(0, 6));

      // Mode & Speed
      UIButton temperatureEffectBtn;
      temperatureEffectBtn.SetName("Effect & Speed");
      temperatureEffectBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(temperature_color, temperature_effect, temperature_effect_period.Get(), start_time); });
      temperatureEffectBtn.OnPress([&]() -> void { EffectModeAndSpeedMenu(Temperature); });
      temperatureEffectBtn.ShouldEnable([&]() -> bool { return mode == Temperature; });
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
  uint16_t period = (mode == RGB) ? rgb_effect_period : temperature_effect_period;

  period = period / 100 - 2;

  start_time = MatrixOS::SYS::Millis();

  UIButton staticBtn;
  staticBtn.SetName("Static");
  staticBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, Static, period * 100 + 200, start_time).DimIfNot(effectMode == Static); });
  staticBtn.OnPress([&]() -> void { effectMode = Static; });
  effectUI.AddUIComponent(staticBtn, Point(2, 0));

  UIButton breathBtn;
  breathBtn.SetName("Breathing");
  breathBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, Breath, period * 100 + 200, start_time).DimIfNot(effectMode == Breath); });
  breathBtn.OnPress([&]() -> void { effectMode = Breath; });
  effectUI.AddUIComponent(breathBtn, Point(3, 0));

  UIButton strobeBtn;
  strobeBtn.SetName("Strobe");
  strobeBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, Strobe, period * 100 + 200, start_time).DimIfNot(effectMode == Strobe); });
  strobeBtn.OnPress([&]() -> void { effectMode = Strobe; });
  effectUI.AddUIComponent(strobeBtn, Point(4, 0));

  UIButton sawBtn;
  sawBtn.SetName("Saw");
  sawBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(color, Saw, period * 100 + 200, start_time).DimIfNot(effectMode == Saw); });
  sawBtn.OnPress([&]() -> void { effectMode = Saw; });
  effectUI.AddUIComponent(sawBtn, Point(5, 0));

  UISelector speedSelector(Dimension(8, 2), "Speed Selector", color, 16, &period);
  effectUI.AddUIComponent(speedSelector, Point(0, 6));

  effectUI.Start();

  // Post exit save variable
  if(mode == RGB)
  {
    rgb_effect = effectMode;
    rgb_effect_period = period * 100 + 200;
  }
  else if(mode == Temperature)
  {
    temperature_effect = effectMode;
    temperature_effect_period = period * 100 + 200;
  }
}