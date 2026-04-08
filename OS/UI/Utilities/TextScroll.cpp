#include "MatrixOS.h"
#include "UI/UI.h"
#include "../Data/font.h"

#define TEXT_SCROLL_SPACING 2

namespace MatrixOS::UIUtility
{
// This function displays a given string on the LED screen in a given color, at a given scroll speed, optionally
// looping
void TextScrollSlow(string text, Color color, uint16_t speed, bool loop) {
  // Log the text we're about to scroll
  MLOGD("Text Scroll Slow", "Printing %s", text.c_str());

  if (Device::ySize < 8)
  {
    MLOGE("Text Scroll Slow", "Not enough vertical space, abort");
    return;
  }

  // Text Scroll Timer
  Timer textScrollTimer;

  // Create a new layer on the LED screen
  MatrixOS::LED::CreateLayer();

  // Create a buffer to store the state of each LED
  bool buffer[Device::xSize][Device::ySize]; // TODO Check Device Rotation

  // Clear the buffer
  for (uint8_t x = 0; x < Device::xSize; x++)
  {
    for (uint8_t y = 0; y < Device::ySize; y++)
    {
      buffer[x][y] = 0;
    }
  }

  // Convert the scroll speed from frames per second to milliseconds per frame
  speed = 1000 / speed;

  // Main loop (if loop == true)
  do
  {
    // Iterate through each character in the string
    for (uint8_t i = 0; i < text.length() + 1; i++)
    {
      // Save the current character into a variable
      char currentChar;
      if (i < text.length())
      {
        currentChar = text[i];
      }
      else
      {
        currentChar = ' '; // TODO add proper ending spacing so x width is not limited to 8x8
      }

      // MLOGD("Text Scroll Slow", "Printing %c", currentChar);
      if (currentChar < 127) // Check ASCII is within bound
      {
        if (currentChar < 32) // Speed change control characters
        {
          speed = currentChar * 10 + 10;
          break;
        }
        else // Regular text
        {
          // Iterate through each column in the character
          for (uint8_t currentCharProgress = 0; currentCharProgress < font8[currentChar - 32][0] + TEXT_SCROLL_SPACING;
               currentCharProgress++)
          {
            // Shift the buffer to the left
            for (uint8_t x = 1; x < Device::xSize; x++)
            {
              memcpy(buffer[x - 1], buffer[x], 8);
            }

            if (currentCharProgress < font8[currentChar - 32][0]) // Render Text column
            {
              // Iterate through each row in the character
              for (uint8_t y = 0; y < 8; y++)
              {
                // Set the state of the LED at the end of the buffer to the corresponding bit in the character's
                // column
                buffer[Device::xSize - 1][y] = bitRead(font8[currentChar - 32][currentCharProgress + 1], 7 - y);
              }
            }
            else // Render Spacing
            {
              for (uint8_t y = 0; y < 8; y++)
              {
                buffer[Device::xSize - 1][y] = 0;
              }
            }

            // Render the buffer to the LED screen
            for (uint8_t x = 0; x < Device::xSize; x++)
            {
              for (uint8_t y = 0; y < 8; y++)
              {
                MatrixOS::LED::SetColor(Point(x, y), buffer[x][y] ? color : Color(0));
              }
            }
            MatrixOS::LED::Update();

            // Wait for the next frame
            while (!textScrollTimer.Tick(speed))
            {
              UI::GlobalLoops();
              MatrixOS::Input::ClearQueue();
              InputSnapshot fnSnap;
              if (MatrixOS::Input::GetState(MatrixOS::Input::GetFunctionKeyId(), &fnSnap) &&
                  fnSnap.keypad.state == KeypadState::Pressed)
              {
                MatrixOS::Input::ClearQueue();
                MatrixOS::LED::DestroyLayer();
                MatrixOS::LED::Update();
                return;
              }
            }
          }
        }
      }
      currentChar++;
    }
  } while (loop);

  // Exit
  MatrixOS::LED::DestroyLayer();
  MatrixOS::LED::Update();
  return;
}

// This function displays a given string on the LED screen in a given color, at a given scroll speed, optionally
// looping
void TextScrollFast(string text, Color color, uint16_t speed, bool loop) {
  // Log the text we're about to scroll
  MLOGD("Text Scroll Fast", "Printing %s", text.c_str());

  if (Device::ySize < 8)
  {
    MLOGE("Text Scroll Fast", "Not enough vertical space, abort");
    return;
  }

  // Text Scroll Timer
  Timer textScrollTimer;

  // Create a new layer on the LED screen
  MatrixOS::LED::CreateLayer();

  // Create a buffer to store the state of each LED
  bool buffer[Device::xSize][Device::ySize]; // TODO Check Device Rotation

  // Clear the buffer
  for (uint8_t x = 0; x < Device::xSize; x++)
  {
    for (uint8_t y = 0; y < Device::ySize; y++)
    {
      buffer[x][y] = 0;
    }
  }

  // Convert the scroll speed from frames per second to milliseconds per frame
  speed = 500 / speed;

  // Main loop (if loop == true)
  do
  {
    // Iterate through each character in the string
    for (uint8_t i = 0; i < text.length() + 1; i++)
    {
      // Save the current character into a variable
      char currentChar;
      if (i < text.length())
      {
        currentChar = text[i];
      }
      else
      {
        currentChar = ' '; // TODO add proper ending spacing so x width is not limited to 8x8
      }

      // MLOGD("Text Scroll Fast", "Printing %c", currentChar);
      if (currentChar < 127) // Check ASCII is within bound
      {
        if (currentChar < 32) // Speed change control characters
        {
          speed = currentChar * 10 + 10;
          break;
        }
        else // Regular text
        {
          // Left shift buffer by font8[currentChar - 32][0] + TEXT_SCROLL_SPACING
          // for (uint8_t x = 0; x < Device::xSize; x++)
          // {
          //   for (uint8_t y = 0; y < 8; y++)
          //   {
          //     if ((x + font8[currentChar - 32][0] + TEXT_SCROLL_SPACING) < Device::xSize)
          //     {
          //       buffer[x][y] = buffer[x + font8[currentChar - 32][0] + TEXT_SCROLL_SPACING][y];
          //     }
          //     else
          //     {
          //       buffer[x][y] = 0;
          //     }
          //   }
          // }

          // Clear the buffer
          for (uint8_t x = 0; x < Device::xSize; x++)
          {
            for (uint8_t y = 0; y < Device::ySize; y++)
            {
              buffer[x][y] = 0;
            }
          }

          // Render the buffer to the LED screen
          for (uint8_t x = 0; x < Device::xSize; x++)
          {
            for (uint8_t y = 0; y < 8; y++)
            {
              MatrixOS::LED::SetColor(Point(x, y), buffer[x][y] ? color : Color(0));
            }
          }
          MatrixOS::LED::Update();

          // Wait for the next frame
          while (!textScrollTimer.Tick(speed))
          {
            UI::GlobalLoops();
            MatrixOS::Input::ClearQueue();
            InputSnapshot fnSnap;
            if (MatrixOS::Input::GetState(MatrixOS::Input::GetFunctionKeyId(), &fnSnap) &&
                fnSnap.keypad.state == KeypadState::Pressed)
            {
              MatrixOS::Input::ClearQueue();
              MatrixOS::LED::DestroyLayer();
              MatrixOS::LED::Update();
              return;
            }
          }

          // Draw new character
          for (uint8_t x = 0; x < font8[currentChar - 32][0]; x++)
          {
            uint8_t canvasX = Device::xSize - font8[currentChar - 32][0] + x - 1;
            for (uint8_t y = 0; y < 8; y++)
            {
              buffer[canvasX][y] = bitRead(font8[currentChar - 32][x + 1], 7 - y);
            }
          }

          // Render the buffer to the LED screen
          for (uint8_t x = 0; x < Device::xSize; x++)
          {
            for (uint8_t y = 0; y < 8; y++)
            {
              MatrixOS::LED::SetColor(Point(x, y), buffer[x][y] ? color : Color(0));
            }
          }
          MatrixOS::LED::Update();

          // Wait for 4 ticks
          while (!textScrollTimer.Tick(speed * 4))
          {
            UI::GlobalLoops();
            MatrixOS::Input::ClearQueue();
            InputSnapshot fnSnap;
            if (MatrixOS::Input::GetState(MatrixOS::Input::GetFunctionKeyId(), &fnSnap) &&
                fnSnap.keypad.state == KeypadState::Pressed)
            {
              MatrixOS::Input::ClearQueue();
              MatrixOS::LED::DestroyLayer();
              MatrixOS::LED::Update();
              return;
            }
          }
        }
      }
      currentChar++;
    }
  } while (loop);

  // Exit
  MatrixOS::LED::DestroyLayer();
  MatrixOS::LED::Update();
  return;
}

void TextScroll(string text, Color color, uint16_t speed, bool loop) {

  if (MatrixOS::UserVar::fastScroll)
  {
    TextScrollFast(text, color, speed, loop);
  }
  else
  {
    TextScrollSlow(text, color, speed, loop);
  }
}
} // namespace MatrixOS::UIUtility