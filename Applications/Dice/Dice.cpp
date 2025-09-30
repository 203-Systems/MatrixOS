#include "Dice.h"
#include "UI/UI.h"  // Include the UI Framework

// Run once
void Dice::Setup(const vector<string>& args) {
  #ifdef FAMILY_MYSTRIX
  if(Device::deviceInfo.Model[3] == 'P'){
    underglow_enabled = true;
  }
  else if(Device::deviceInfo.Model[3] == 'S'){
    underglow_enabled = false;
  }
#endif
  rolling_start_time = MatrixOS::SYS::Millis();
  last_roll_time = MatrixOS::SYS::Millis();
  current_phase = Rolling;
  srand(MatrixOS::SYS::Millis());
  RollDice();
}

// Run in a loop after Setup()
void Dice::Loop() {
  // Set up key event handler
  struct KeyEvent keyEvent;                 // Variable for the latest key event to be stored at
  while (MatrixOS::KeyPad::Get(&keyEvent))  // While there is still keyEvent in the queue
  {
    KeyEventHandler(keyEvent);
  }  // Handle them

  if (!renderTimer.Tick(1000 / Device::LED::fps))
  {
    return;
  }  // Render at 100fps

  if (current_phase == Rolling)
  {
    if ((MatrixOS::SYS::Millis() - last_roll_time) >= (10 * flashing_speed))
    {
      RollDice();
    }

    if ((MatrixOS::SYS::Millis() - rolling_start_time) >= (100 * rolling_speed))
    {
      current_phase = Confirmed;
    }
    else
    {
      Color color = rolling_rainbow_mode ? ColorEffects::Rainbow() : rolling_color;
      if (mode == Dot)
      {
        RenderDots(rolled_number, color);
      }
      else if(mode == Number)
      {
        RenderNumbers(rolled_number, color);
      }
      RenderUnderglow(rolling_underglow_mode, color, rolling_underglow_effect_period);
    }
  }

  if (current_phase == Confirmed)
  {
    Color color = confirmed_rainbow_mode ? ColorEffects::Rainbow(3000) : confirmed_color;
    if (mode == Dot)
    {
      RenderDots(rolled_number, color);
    }
    else if(mode == Number)
    {
      RenderNumbers(rolled_number, color);
    }
    RenderUnderglow(confirmed_underglow_mode, color, confirmed_underglow_effect_period);
  }

  MatrixOS::LED::Update();  // Update the LED
}

// Handle the key event from the OS
void Dice::KeyEventHandler(KeyEvent& keyEvent) {
  if (keyEvent.ID() == FUNCTION_KEY)  // FUNCTION_KEY is pre defined by the device, as the keyID for the system function key
  {
    if (keyEvent.State() == HOLD)
    {
      Settings();  // Open UI Menu
    }
    else if (keyEvent.State() == RELEASED)  // If the function key is released and not hold
    {
      rolling_start_time = MatrixOS::SYS::Millis();
      current_phase = Rolling;
      RollDice();
    }
  }
}

void Dice::Settings() {
  // Matrix OS Debug Log, sent to hardware UART and USB CDC
  //   MLOGI("Example", "Enter UI Menu");

  //   // Create a UI Object
  //   // UI Name, Color (as the text scroll color). and new led layer (Set as true, the UI will render on a new led layer. Persevere what was rendered before after UI exits)
  UI settingsUI("Settings", Color(0x00FFFF), true);

  UIButton rollingColorSelectorBtn;
  rollingColorSelectorBtn.SetName("Rolling Color");
  rollingColorSelectorBtn.SetColorFunc([&]() -> Color { return rolling_rainbow_mode ? ColorEffects::Rainbow() : rolling_color.Get(); });
  rollingColorSelectorBtn.SetSize(Dimension(1, 4));
  rollingColorSelectorBtn.OnPress([&]() -> void {
    if (!rolling_rainbow_mode)
    {
      MatrixOS::UIUtility::ColorPicker(rolling_color.Get());
      rolling_color.Save();
    }
  });

  settingsUI.AddUIComponent(rollingColorSelectorBtn, Point(0, 2));

  UIToggle rollingRainbowModeToggle;
  rollingRainbowModeToggle.SetName("Rolling Rainbow Mode");
  rollingRainbowModeToggle.SetColorFunc([&]() -> Color { return ColorEffects::Rainbow(); });
  rollingRainbowModeToggle.SetValuePointer(&rolling_rainbow_mode);
  rollingRainbowModeToggle.OnPress([&]() -> void { rolling_rainbow_mode.Save(); });
  settingsUI.AddUIComponent(rollingRainbowModeToggle, Point(0, 0));
  
  UIButton rollingUnderglowEffectMenuBtn;
  rollingUnderglowEffectMenuBtn.SetName("Rolling Underglow Effect");
  rollingUnderglowEffectMenuBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(rolling_rainbow_mode ? ColorEffects::Rainbow() : rolling_color.Get(), rolling_underglow_mode, rolling_underglow_effect_period, timestamp); });
  rollingUnderglowEffectMenuBtn.OnPress([&]() -> void { UnderglowEffectModeAndSpeedMenu(Rolling); });
  rollingUnderglowEffectMenuBtn.SetEnabled(underglow_enabled);
  settingsUI.AddUIComponent(rollingUnderglowEffectMenuBtn, Point(0, 7));

  UIButton confirmedColorSelectorBtn;
  confirmedColorSelectorBtn.SetName("Confirmed Color");
  confirmedColorSelectorBtn.SetColorFunc([&]() -> Color { return confirmed_rainbow_mode ? ColorEffects::Rainbow() : confirmed_color.Get(); });
  confirmedColorSelectorBtn.SetSize(Dimension(1, 4));
  confirmedColorSelectorBtn.OnPress([&]() -> void {
    if (!confirmed_rainbow_mode)
    {
      MatrixOS::UIUtility::ColorPicker(confirmed_color.Get());
      confirmed_color.Save();
    }
  });
  settingsUI.AddUIComponent(confirmedColorSelectorBtn, Point(7, 2));

  UIToggle confirmedRainbowModeToggle;
  confirmedRainbowModeToggle.SetName("Confirmed Rainbow Mode");
  confirmedRainbowModeToggle.SetColorFunc([&]() -> Color { return ColorEffects::Rainbow(); });
  confirmedRainbowModeToggle.SetValuePointer(&confirmed_rainbow_mode);
  confirmedRainbowModeToggle.OnPress([&]() -> void { confirmed_rainbow_mode.Save(); });
  settingsUI.AddUIComponent(confirmedRainbowModeToggle, Point(7, 0));
  

  UIButton confirmedUnderglowEffectMenuBtn;
  confirmedUnderglowEffectMenuBtn.SetName("Confirmed Underglow Effect");
  confirmedUnderglowEffectMenuBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(confirmed_rainbow_mode ? ColorEffects::Rainbow() : confirmed_color.Get(), confirmed_underglow_mode, confirmed_underglow_effect_period, timestamp); });
  confirmedUnderglowEffectMenuBtn.OnPress([&]() -> void { UnderglowEffectModeAndSpeedMenu(Confirmed); });
  confirmedUnderglowEffectMenuBtn.SetEnabled(underglow_enabled);
  settingsUI.AddUIComponent(confirmedUnderglowEffectMenuBtn, Point(7, 7));

  UIButton dotModeToggle;
  dotModeToggle.SetName("Dot Mode");
  dotModeToggle.SetColorFunc([&]() -> Color { return Color(0xFF00FF).DimIfNot(mode == Dot); });
  dotModeToggle.OnPress([&]() -> void { mode = Dot; });
  settingsUI.AddUIComponent(dotModeToggle, Point(3, 0));

  UIButton numberModeToggle;
  numberModeToggle.SetName("Number Mode");
  numberModeToggle.SetColorFunc([&]() -> Color { return Color(0xFF5000).DimIfNot(mode == Number); });
  numberModeToggle.OnPress([&]() -> void { mode = Number; });
  settingsUI.AddUIComponent(numberModeToggle, Point(4, 0));

  // Dot Mode
  UIButton dotFaceSelectorBtn;
  dotFaceSelectorBtn.SetName("Faces");
  dotFaceSelectorBtn.SetColor(Color(0x00FFFF));
  dotFaceSelectorBtn.SetSize(Dimension(4, 1));
  dotFaceSelectorBtn.OnPress([&]() -> void { DotFaceSelector(); });
  dotFaceSelectorBtn.SetEnableFunc([&]() -> bool { return mode == Dot; });
  settingsUI.AddUIComponent(dotFaceSelectorBtn, Point(2, 7));

  // Number Mode
  UIButton numberFacesSelectorBtn;
  numberFacesSelectorBtn.SetName("Faces");
  numberFacesSelectorBtn.SetColor(Color(0x00FFFF));
  numberFacesSelectorBtn.SetSize(Dimension(4, 1));
  numberFacesSelectorBtn.OnPress([&]() -> void { number_faces = MatrixOS::UIUtility::NumberSelector8x8(number_faces, Color(0x00FFFF), "Face Selector", 1, 99); });
  numberFacesSelectorBtn.SetEnableFunc([&]() -> bool { return mode == Number; });
  settingsUI.AddUIComponent(numberFacesSelectorBtn, Point(2, 7));

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

  //   // The UI object is now fully set up. Let the UI runtime to start and take over.
  settingsUI.Start();
}

uint8_t Dice::GetRandomNumber(uint8_t upperbound, uint8_t lowerbound) {
  return rand() % (upperbound - lowerbound + 1) + lowerbound;
}

void Dice::RenderDot(Point point, Color color) {
  MatrixOS::LED::SetColor(point, color);
  MatrixOS::LED::SetColor(point + Point(1, 0), color);
  MatrixOS::LED::SetColor(point + Point(0, 1), color);
  MatrixOS::LED::SetColor(point + Point(1, 1), color);
}

void Dice::RenderDots(uint8_t number, Color color) {
  MatrixOS::LED::Fill(0);  // Clear the LED
  if (number > 9)
  {
    return;
  }

  // Render the dots
  switch (number)
  {
    case 1:
      RenderDot(Point(3, 3), color);
      break;
    case 2:
      RenderDot(Point(1, 3), color);
      RenderDot(Point(5, 3), color);
      break;
    case 3:
      RenderDot(Point(0, 3), color);
      RenderDot(Point(3, 3), color);
      RenderDot(Point(6, 3), color);
      break;
    case 4:
      RenderDot(Point(1, 1), color);
      RenderDot(Point(5, 1), color);
      RenderDot(Point(1, 5), color);
      RenderDot(Point(5, 5), color);
      break;
    case 5:
      RenderDot(Point(1, 1), color);
      RenderDot(Point(5, 1), color);
      RenderDot(Point(1, 5), color);
      RenderDot(Point(5, 5), color);
      RenderDot(Point(3, 3), color);
      break;
    case 6:
      RenderDot(Point(0, 1), color);
      RenderDot(Point(3, 1), color);
      RenderDot(Point(6, 1), color);
      RenderDot(Point(0, 5), color);
      RenderDot(Point(3, 5), color);
      RenderDot(Point(6, 5), color);
      break;
    case 7:
      RenderDot(Point(0, 0), color);
      RenderDot(Point(3, 0), color);
      RenderDot(Point(6, 0), color);
      RenderDot(Point(3, 3), color);
      RenderDot(Point(0, 6), color);
      RenderDot(Point(3, 6), color);
      RenderDot(Point(6, 6), color);
      break;
    case 8:
      RenderDot(Point(0, 0), color);
      RenderDot(Point(3, 0), color);
      RenderDot(Point(6, 0), color);
      RenderDot(Point(0, 3), color);
      RenderDot(Point(6, 3), color);
      RenderDot(Point(0, 6), color);
      RenderDot(Point(3, 6), color);
      RenderDot(Point(6, 6), color);
      break;
    case 9:
      RenderDot(Point(0, 0), color);
      RenderDot(Point(3, 0), color);
      RenderDot(Point(6, 0), color);
      RenderDot(Point(0, 3), color);
      RenderDot(Point(3, 3), color);
      RenderDot(Point(6, 3), color);
      RenderDot(Point(0, 6), color);
      RenderDot(Point(3, 6), color);
      RenderDot(Point(6, 6), color);
      break;
  }
}

void Dice::RenderNumber(Point point, uint8_t number, Color color) {
  for (uint8_t x = 0; x < 4; x++)
  {
    for (uint8_t y = 0; y < 6; y++)
    {
      if (number_font[number * 4 + x] & (1 << y))
      {
        MatrixOS::LED::SetColor(point + Point(x, y), color);
      }
    }
  }
}

void Dice::RenderNumbers(uint8_t number, Color color) {
  MatrixOS::LED::Fill(0);  // Fill the LED with the color
  if (number > 99)
  {
    return;
  }  // If the number is greater than 99, abort

  uint8_t digit1 = number / 10;  // Get the first digit
  uint8_t digit2 = number % 10;  // Get the second digit

  if (digit1 > 0)  // If the first digit is greater than 0
  {
    RenderNumber(Point(0, 1), digit1, color);  // Render the first digit
    RenderNumber(Point(4, 1), digit2, color);  // Render the second digit
  }
  else
  {
    RenderNumber(Point(2, 1), digit2, color);  // Render the second digit
  }
}

void Dice::RollDice() {
  uint8_t faces = 0;

  if(mode == Dot)
  {
    faces = dot_faces;
  }
  else if(mode == Number)
  {
    faces = number_faces;
  }
  last_roll_time = MatrixOS::SYS::Millis();
  uint8_t last_rolled_number = rolled_number;

  do
  {
    rolled_number = GetRandomNumber(faces);
  } while (rolled_number == last_rolled_number);
}

void Dice::RenderUnderglow(UnderglowEffectMode mode, Color color, uint16_t period) {
  if (!underglow_enabled) { return; }
  MatrixOS::LED::FillPartition("Underglow", ApplyColorEffect(color, mode, period, timestamp), 1);
}

Color Dice::ApplyColorEffect(Color color, UnderglowEffectMode effect, uint16_t period, uint16_t start_time) {
  switch (effect)
  {
    case Static:
      break;
    case Off:
      color = Color(0);
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

void Dice::DotFaceSelector() {
  int32_t dot_faces_num = dot_faces.Get();
  int32_t selector_dot_faces = dot_faces_num - 2;
  UI dotFaceSelector("Face Selector", Color(0x00FFFF));

  UI4pxNumber numDisplay;
  numDisplay.SetColor(Color(0x00FFFF));
  numDisplay.SetDigits(1);
  numDisplay.SetValuePointer((int32_t*)&dot_faces_num);
  dotFaceSelector.AddUIComponent(numDisplay, Point(5, 0));
  

  UISelector overlapInput;
  overlapInput.SetDimension(Dimension(8, 1));
  overlapInput.SetName("Faces");
  overlapInput.SetColor(Color(0x00FFFF));
  overlapInput.SetCount(8);
  overlapInput.SetValuePointer((uint16_t*)&selector_dot_faces);
  overlapInput.OnChange([&](uint16_t val) -> void { dot_faces_num = val + 2; });
  dotFaceSelector.AddUIComponent(overlapInput, Point(0, 7));

  dotFaceSelector.Start();
  dot_faces = dot_faces_num;
}

void Dice::UnderglowEffectModeAndSpeedMenu(DicePhase phase) {
  Color color;
  bool rainbow = false;
  uint16_t period;
  UnderglowEffectMode effectMode;

  if (phase == Rolling)
  {
    color = rolling_color.Get();
    rainbow = rolling_rainbow_mode;
    period = rolling_underglow_effect_period;
    effectMode = rolling_underglow_mode;
  }
  else if (phase == Confirmed)
  {
    color = confirmed_color.Get();
    rainbow = confirmed_rainbow_mode;
    period = confirmed_underglow_effect_period;
    effectMode = confirmed_underglow_mode;
  }

  UI effectUI("Underglow Effect Mode", Color(color), true);

  period = period / 100 - 2;

  timestamp = MatrixOS::SYS::Millis();

  UIButton enableBtn;
  enableBtn.SetName("Static");
  enableBtn.SetColorFunc([&]() -> Color { return Color(0x00FF00).DimIfNot(effectMode != Off); });
  enableBtn.OnPress([&]() -> void {
    if (effectMode == Off)
    {
      effectMode = Static;
    }
    else
    {
      effectMode = Off;
    }
  });
  effectUI.AddUIComponent(enableBtn, Point(0, 0));

  UIButton staticBtn;
  staticBtn.SetName("Static");
  staticBtn.SetColorFunc([&]() -> Color {
    return ApplyColorEffect(rainbow ? ColorEffects::Rainbow() : color, Static, period * 100 + 200, timestamp).DimIfNot(effectMode == Static);
  });
  staticBtn.OnPress([&]() -> void { effectMode = Static; });
  effectUI.AddUIComponent(staticBtn, Point(2, 0));

  UIButton breathBtn;
  breathBtn.SetName("Breathing");
  breathBtn.SetColorFunc([&]() -> Color {
    return ApplyColorEffect(rainbow ? ColorEffects::Rainbow() : color, Breath, period * 100 + 200, timestamp).DimIfNot(effectMode == Breath);
  });
  breathBtn.OnPress([&]() -> void { effectMode = Breath; });
  effectUI.AddUIComponent(breathBtn, Point(3, 0));

  UIButton strobeBtn;
  strobeBtn.SetName("Strobe");
  strobeBtn.SetColorFunc([&]() -> Color {
    return ApplyColorEffect(rainbow ? ColorEffects::Rainbow() : color, Strobe, period * 100 + 200, timestamp).DimIfNot(effectMode == Strobe);
  });
  strobeBtn.OnPress([&]() -> void { effectMode = Strobe; });
  effectUI.AddUIComponent(strobeBtn, Point(4, 0));

  UIButton sawBtn;
  sawBtn.SetName("Saw");
  sawBtn.SetColorFunc([&]() -> Color { return ApplyColorEffect(rainbow ? ColorEffects::Rainbow() : color, Saw, period * 100 + 200, timestamp).DimIfNot(effectMode == Saw); });
  sawBtn.OnPress([&]() -> void { effectMode = Saw; });
  effectUI.AddUIComponent(sawBtn, Point(5, 0));

  UISelector speedSelector;
  speedSelector.SetDimension(Dimension(8, 2));
  speedSelector.SetName("Speed Selector");
  speedSelector.SetColor(color);
  speedSelector.SetCount(16);
  speedSelector.SetValuePointer(&period);
  effectUI.AddUIComponent(speedSelector, Point(0, 6));

  effectUI.Start();

  // Post exit save variable
  if (phase == Rolling)
  {
    rolling_underglow_mode = effectMode;
    rolling_underglow_effect_period = period * 100 + 200;
  }
  else if (phase == Confirmed)
  {
    confirmed_underglow_mode = effectMode;
    confirmed_underglow_effect_period = period * 100 + 200;
    
  }
}