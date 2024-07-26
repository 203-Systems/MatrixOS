#include "Dice.h"
#include "ui/UI.h" // Include the UI Framework
#include "applications/BrightnessControl/BrightnessControl.h"


// Run once
void Dice::Setup() {
  rolling_start_time = MatrixOS::SYS::Millis();
  last_roll_time = MatrixOS::SYS::Millis();
  current_phase = Rolling;
  srand(MatrixOS::SYS::Millis());
  RollDice();
}

// Run in a loop after Setup()
void Dice::Loop() {
  // Set up key event handler
  struct KeyEvent keyEvent; // Variable for the latest key event to be stored at
  while (MatrixOS::KEYPAD::Get(&keyEvent)) // While there is still keyEvent in the queue
  { KeyEventHandler(keyEvent.id, &keyEvent.info); } // Handle them

  if(!renderTimer.Tick(1000 / Device::fps)){return;} // Render at 100fps

  if(current_phase == Rolling)
  {
    if((MatrixOS::SYS::Millis() - last_roll_time) >= (10 * flashing_speed))
    {
      RollDice();
    }

    if((MatrixOS::SYS::Millis() - rolling_start_time) >= (100 * rolling_speed))
    {
      current_phase = Comfirmed;
    }
    else
    {
      Color color = rolling_rainbow_mode ? ColorEffects::Rainbow() : rolling_color;
      if(number_view || faces >= 10)
      {
        RenderNumbers(rolled_number, color);
      }
      else
      {
        RenderDots(rolled_number, color);
      }
      RenderUnderglow(rolling_underglow_mode, color, rolling_underglow_color_speed);
    }
  }
  
  if(current_phase == Comfirmed)
  {
    Color color = comfirmed_rainbow_mode ? ColorEffects::Rainbow(3000) : comfirmed_color;
    if(number_view || faces >= 10)
    {
      RenderNumbers(rolled_number, color);
    }
    else
    {
      RenderDots(rolled_number, color);
    }
    RenderUnderglow(comfirmed_underglow_mode, color, comfirmed_underglow_color_speed);
  }

  MatrixOS::LED::Update(); // Update the LED
}

// Handle the key event from the OS
void Dice::KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo) {
    if (keyID == FUNCTION_KEY)  // FUNCTION_KEY is pre defined by the device, as the keyID for the system function key
    {
      if(keyInfo->state == HOLD)
      {
          Settings();                 // Open UI Menu
      }
      else if(keyInfo->state == RELEASED && keyInfo->hold == false)  // If the function key is released and not hold
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

  UIButtonLarge brightnessBtn(
      "Brightness", Color(0xFFFFFF), Dimension(2, 2), [&]() -> void { MatrixOS::LED::NextBrightness(); }, [&]() -> void { BrightnessControl().Start(); });
  settingsUI.AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButtonLarge rotateUpBtn("This does nothing", Color(0x00FF00), Dimension(2, 1), [&]() -> void {});
  settingsUI.AddUIComponent(rotateUpBtn, Point(3, 2));

  UIButtonLarge rotatRightBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2), [&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  settingsUI.AddUIComponent(rotatRightBtn, Point(5, 3));

  UIButtonLarge rotateDownBtn("Rotate to this side", Color(0x00FF00), Dimension(2, 1), [&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  settingsUI.AddUIComponent(rotateDownBtn, Point(3, 5));

  UIButtonLarge rotateLeftBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2), [&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  settingsUI.AddUIComponent(rotateLeftBtn, Point(2, 3));

  // Color
  UIButtonLargeWithColorFunc rollingColorSelectorBtn(
      "Rolling Color", [&]() -> Color { return rolling_rainbow_mode ? ColorEffects::Rainbow() : rolling_color; }, Dimension(1, 4),
      [&]() -> void {
        if (!rolling_rainbow_mode)
        {
          MatrixOS::UIInterface::ColorPicker(rolling_color.Get());
          rolling_color.Save();
        }
      });

  settingsUI.AddUIComponent(rollingColorSelectorBtn, Point(0, 2));

  UIButtonWithColorFunc rollingRainbowModeToggle(
      "Rolling Rainbow Mode", [&]() -> Color { return ColorEffects::Rainbow().ToLowBrightness(rolling_rainbow_mode); }, [&]() -> void { rolling_rainbow_mode = !rolling_rainbow_mode; });
  settingsUI.AddUIComponent(rollingRainbowModeToggle, Point(0, 7));

  UIButtonLargeWithColorFunc comfirmedColorSelectorBtn(
      "Comfirmed Color", [&]() -> Color { return comfirmed_rainbow_mode ? ColorEffects::Rainbow() : comfirmed_color; }, Dimension(1, 4),
      [&]() -> void { if(!comfirmed_rainbow_mode){MatrixOS::UIInterface::ColorPicker(comfirmed_color.Get());
      comfirmed_color.Save();
    } });
  settingsUI.AddUIComponent(comfirmedColorSelectorBtn, Point(7, 2));

  UIButtonWithColorFunc comfirmedRainbowModeToggle(
      "Comfirmed Rainbow Mode", [&]() -> Color { return ColorEffects::Rainbow().ToLowBrightness(comfirmed_rainbow_mode); }, [&]() -> void { comfirmed_rainbow_mode = !comfirmed_rainbow_mode; });
  settingsUI.AddUIComponent(comfirmedRainbowModeToggle, Point(7, 7));

  UIButtonLargeWithColorFunc numberViewToggle(
      "Number View", [&]() -> Color { return Color(0xFF5000).ToLowBrightness(faces < 10 && number_view); }, Dimension(4, 1),
      [&]() -> void {
        if (faces < 10)
        {
          number_view = !number_view;
        }
      });
  settingsUI.AddUIComponent(numberViewToggle, Point(2, 7));

  int32_t modifier[8] = {-10, -5, -2, -1, 1, 2, 5, 10};
  UIButtonLarge facesSelectorBtn("Faces", Color(0x00FFFF), Dimension(4, 1), [&]() -> void {faces = MatrixOS::UIInterface::NumberSelector8x8(faces, Color(0x00FFFF), "Face Selector", 1, 99);});
  settingsUI.AddUIComponent(facesSelectorBtn, Point(2, 0));

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


uint8_t Dice::GetRandomNumber(uint8_t upperbound, uint8_t lowerbound)
{
  return rand() % (upperbound - lowerbound + 1) + lowerbound;
}

void Dice::RenderDot(Point point, Color color)
{
  MatrixOS::LED::SetColor(point, color);
  MatrixOS::LED::SetColor(point + Point(1, 0), color);
  MatrixOS::LED::SetColor(point + Point(0, 1), color);
  MatrixOS::LED::SetColor(point + Point(1, 1), color);
}

void Dice::RenderDots(uint8_t number, Color color)
{
  MatrixOS::LED::Fill(0); // Clear the LED
  if(number > 9) {
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

void Dice::RenderNumber(Point point, uint8_t number, Color color)
{
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

void Dice::RenderNumbers(uint8_t number, Color color)
{
  MatrixOS::LED::Fill(0); // Fill the LED with the color
  if (number > 99) { return; } // If the number is greater than 99, abort

  uint8_t digit1 = number / 10; // Get the first digit
  uint8_t digit2 = number % 10; // Get the second digit

  if (digit1 > 0) // If the first digit is greater than 0
  {
    RenderNumber(Point(0, 1), digit1, color); // Render the first digit
    RenderNumber(Point(4, 1), digit2, color); // Render the second digit
  }
  else
  {
    RenderNumber(Point(2, 1), digit2, color); // Render the second digit
  }
}

void Dice::RollDice()
{
  last_roll_time = MatrixOS::SYS::Millis();
  rolled_number = GetRandomNumber(faces);
}

void Dice::RenderUnderglow(UnderglowEffectMode mode, Color color, uint8_t period)
{
  switch (mode)
  {
  case Static:
    MatrixOS::LED::FillPartition("Underglow", color, 1);
    break;
  case Off:
    MatrixOS::LED::FillPartition("Underglow", 0, 1);
    break;
  case Breath:
    MatrixOS::LED::FillPartition("Underglow", ColorEffects::ColorBreath(color, period * 100), 1);
    break;
  case Strobe:
    MatrixOS::LED::FillPartition("Underglow", ColorEffects::ColorStrobe(color, period * 100), 1);
    break;
  case Saw:
    MatrixOS::LED::FillPartition("Underglow", ColorEffects::ColorSaw(color, period * 100), 1);
    break;
  }
}