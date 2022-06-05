#include "MatrixOS.h"
#include "font.h"

namespace MatrixOS::UIComponent
{
#define TEXT_SCROLL_SPACING 2
    void TextScroll(string text, Color color, uint16_t speed, bool loop)
    {
        MatrixOS::Logging::LogDebug("Text Scroll", "Printing %s", text.c_str());
        Timer textScrollTimer;
        Timer keyscanTimer;
        MatrixOS::LED::Fill(Color(0x000000));
        MatrixOS::LED::Update();

        bool buffer[Device::x_size][Device::y_size]; // TODO Check Device Rotation

        //Clear buffer
        for (uint8_t x = 0; x < Device::x_size; x++)
        {
            for (uint8_t y = 0; y < Device::y_size; y++)
            {
                buffer[x][y] = 0;
            }
        }

        speed = 1000 / speed;

        do //Main loop (if loop == true)
        {
            for (uint8_t i = 0; i < text.length() + 1; i++) //iterate string
            {
                char current_char;
                if (i < text.length())
                {
                    current_char = text[i];
                }
                else
                {
                    current_char = ' '; // TODO add proper ending spacing so x width is not limited to 8x8
                }

                // MatrixOS::Logging::LogDebug("Text Scroll", "Printing %c", current_char);
                if (current_char < 127) // Check ASCII is within bound 
                {
                    if (current_char < 32) // Speed change control characters
                    {
                        speed = current_char * 10 + 10;
                        break;
                    }
                    else // Regular text
                    {
                        for (uint8_t current_char_progress = 0; current_char_progress < font8[current_char - 32][0] + TEXT_SCROLL_SPACING; current_char_progress++) //枚举字体的竖列
                        {
                            //Shift buffer to the left
                            for (uint8_t x = 1; x < Device::x_size; x++)
                            {
                                memcpy(buffer[x - 1], buffer[x], 8);
                            }

                            if (current_char_progress < font8[current_char - 32][0]) // Render Text colume
                            {
                                for (uint8_t y = 0; y < 8; y++)
                                {
                                    buffer[Device::x_size - 1][y] = bitRead(font8[current_char - 32][current_char_progress + 1], 7 - y);
                                }
                            }
                            else //Render Spacing
                            {
                                for (uint8_t y = 0; y < 8; y++)
                                {
                                    buffer[Device::x_size - 1][y] = 0;
                                }
                            }

                            //Render to LED
                            for (uint8_t x = 0; x < Device::x_size; x++)
                            {
                                for (uint8_t y = 0; y < 8; y++)
                                {
                                    MatrixOS::LED::SetColor(Point(x, y), buffer[x][y] ? color : 0);
                                }
                            }
                            MatrixOS::LED::Update();

                            while (!textScrollTimer.Tick(speed)) //Waiting for next frame
                            {
                                // MatrixOS::KEYPAD::Scan(true);
                                MatrixOS::KEYPAD::ClearList(); //Keypad will scan itself after list is cleared
                                // MatrixOS::Logging::LogDebug("Text Scroll", "FN Velocity %d", (uint16_t)MatrixOS::KEYPAD::GetKey(FUNCTION_KEY).state);
                                //Let's assume we don't use FN to trigger a text scroll
                                if (MatrixOS::KEYPAD::GetKey(FUNCTION_KEY).state == PRESSED)
                                {
                                    MatrixOS::KEYPAD::GetKey(FUNCTION_KEY).Clear();
                                    MatrixOS::LED::Fill(Color(0x000000));
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
        
        //Exit
        MatrixOS::LED::Fill(Color(0x000000));
        MatrixOS::LED::Update();
        return;
    }
}