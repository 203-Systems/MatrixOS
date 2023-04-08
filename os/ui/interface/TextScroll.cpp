#include "MatrixOS.h"
#include "../data/font.h"

#define TEXT_SCROLL_SPACING 2

namespace MatrixOS::UIInterface
{
  // This function displays a given string on the LED screen in a given color, at a given scroll speed, optionally
  // looping
  void TextScroll(string text, Color color, uint16_t speed, bool loop) {
    // Log the text we're about to scroll
    MLOGD("Text Scroll", "Printing %s", text.c_str());

    if(Device::y_size < 8)
    {
      MLOGE("Text Scroll", "Not enough vertical space, abort");
      return;
    }

    // Create two timers - one for scrolling and one for scanning the keyboard
    Timer textScrollTimer;
    Timer keyscanTimer;

    // Create a new layer on the LED screen
    MatrixOS::LED::CreateLayer();

    // Create a buffer to store the state of each LED
    bool buffer[Device::x_size][Device::y_size];  // TODO Check Device Rotation

    // Clear the buffer
    for (uint8_t x = 0; x < Device::x_size; x++)
    {
      for (uint8_t y = 0; y < Device::y_size; y++)
      { buffer[x][y] = 0; }
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
        char current_char;
        if (i < text.length())
        { current_char = text[i]; }
        else
        {
          current_char = ' ';  // TODO add proper ending spacing so x width is not limited to 8x8
        }

        // MLOGD("Text Scroll", "Printing %c", current_char);
        if (current_char < 127)  // Check ASCII is within bound
        {
          if (current_char < 32)  // Speed change control characters
          {
            speed = current_char * 10 + 10;
            break;
          }
          else  // Regular text
          {
            // Iterate through each column in the character
            for (uint8_t current_char_progress = 0;
                 current_char_progress < font8[current_char - 32][0] + TEXT_SCROLL_SPACING; current_char_progress++)
            {
              // Shift the buffer to the left
              for (uint8_t x = 1; x < Device::x_size; x++)
              { memcpy(buffer[x - 1], buffer[x], 8); }

              if (current_char_progress < font8[current_char - 32][0])  // Render Text colume
              {
                // Iterate through each row in the character
                for (uint8_t y = 0; y < 8; y++)
                {
                  // Set the state of the LED at the end of the buffer to the corresponding bit in the character's
                  // column
                  buffer[Device::x_size - 1][y] = bitRead(font8[current_char - 32][current_char_progress + 1], 7 - y);
                }
              }
              else  // Render Spacing
              {
                for (uint8_t y = 0; y < 8; y++)
                { buffer[Device::x_size - 1][y] = 0; }
              }

              // Render the buffer to the LED screen
              for (uint8_t x = 0; x < Device::x_size; x++)
              {
                for (uint8_t y = 0; y < 8; y++)
                { MatrixOS::LED::SetColor(Point(x, y), buffer[x][y] ? color : Color(0)); }
              }
              MatrixOS::LED::Update();

              // Wait for the next frame
              while (!textScrollTimer.Tick(speed))
              {
                // MatrixOS::KEYPAD::Scan(true);
                MatrixOS::KEYPAD::ClearList();  // Keypad will scan itself after list is cleared
                // MLOGD("Text Scroll", "FN Velocity %d",
                // (uint16_t)MatrixOS::KEYPAD::GetKey(FUNCTION_KEY).state);
                // Let's assume we don't use FN to trigger a text scroll
                if (MatrixOS::KEYPAD::GetKey(FUNCTION_KEY)->state == PRESSED)
                {
                  MatrixOS::KEYPAD::GetKey(FUNCTION_KEY)->Clear();
                  MatrixOS::KEYPAD::ClearList();
                  MatrixOS::LED::DestoryLayer();
                  MatrixOS::LED::Update();
                  return;
                }
              }
            }
          }
        }
        current_char++;
      }
    } while (loop);

    // Exit
    MatrixOS::LED::DestoryLayer();
    MatrixOS::LED::Update();
    return;
  }
}