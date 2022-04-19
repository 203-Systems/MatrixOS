#include "FactoryTest.h"

void FactoryTest::Setup()
{
    MatrixOS::SYS::SetVariable("brightness", 32);
}


void FactoryTest::Loop()
{
    switch(current_test)
    {
        case 0:
            LEDTest();
            return;
        case 1:
            KeyPadTest();
            return;
        case 2:
            TouchBarTest();
            return;
        case 3:
        default:
            Exit();
    }
}

void FactoryTest::LEDTest()
{
    uint8_t led_index = led_counter % Device::numsOfLED;
    uint8_t led_color_index = led_counter / Device::numsOfLED % (sizeof(colors)/sizeof(Color));
    
    MatrixOS::LED::SetColor(led_index, colors[led_color_index]);
    MatrixOS::LED::Update();
    led_counter ++;
    MatrixOS::SYS::DelayMs(30);
}

void FactoryTest::KeyPadTest()
{
    for(uint8_t x = 0; x < Device::x_size; x++)
    {
        for(uint8_t y = 0; y < Device::y_size; y++)
        {
            KeyInfo keyInfo = MatrixOS::KEYPAD::GetKey(Point(x, y));
            keypad_tested[x][y] |= (bool)keyInfo.velocity;

            if(keyInfo.velocity)
            {
                MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00));
            }
            else
            {
                 MatrixOS::LED::SetColor(Point(x, y), Color(0xFFFFFF * keypad_tested[x][y]));
            }
        }
    }
    MatrixOS::LED::Update();
}

void FactoryTest::TouchBarTest()
{
    //Left
    for(uint8_t i = 0; i < 8; i++)
    {
        Point xy = Point(-1, i);
        Point led_xy = Point(0, i);
        uint8_t tested_index = i;

        KeyInfo keyInfo = MatrixOS::KEYPAD::GetKey(xy);
        touchbar_tested[tested_index] |= (bool)keyInfo.velocity;
        
        MatrixOS::LED::SetColor(led_xy, keyInfo.velocity ? Color(0x00FF00) : Color(0xFFFFFF * touchbar_tested[tested_index]));
    }

    //Right
    for(uint8_t i = 0; i < 8; i++)
    {
        Point xy = Point(8, i);
        Point led_xy = Point(7, i);
        uint8_t tested_index = i + 8;

        KeyInfo keyInfo = MatrixOS::KEYPAD::GetKey(xy);
        touchbar_tested[tested_index] |= (bool)keyInfo.velocity;
        
        MatrixOS::LED::SetColor(led_xy, keyInfo.velocity ? Color(0x00FF00) : Color(0xFFFFFF * touchbar_tested[tested_index]));
    }

    // //Top (When Matrix is rotated)
    // for(uint8_t i = 0; i < 8; i++)
    // {
    //     Point xy = Point(i, -1);
    //     Point led_xy = xy + Point(0, 1);
    //     uint8_t tested_index = i + 16;

    //     KeyInfo keyInfo = MatrixOS::KEYPAD::GetKey(xy);
    //     touchbar_tested[tested_index] |= (bool)keyInfo.velocity;
        
    //     MatrixOS::LED::SetColor(led_xy, keyInfo.velocity ? Color(0x00FF00) : Color(0xFFFFFF * touchbar_tested[tested_index]));
    // }

    // //Bottom (When Matrix is rotated)
    // for(uint8_t i = 0; i < 8; i++)
    // {
    //     Point xy = Point(i, 1);
    //     Point led_xy = Point(i, 0);
    //     uint8_t tested_index = i + 24;

    //     KeyInfo keyInfo = MatrixOS::KEYPAD::GetKey(xy);
    //     touchbar_tested[tested_index] |= (bool)keyInfo.velocity;
        
    //     MatrixOS::LED::SetColor(led_xy, keyInfo.velocity ? Color(0x00FF00) : Color(0xFFFFFF * touchbar_tested[tested_index]));
    // }
    MatrixOS::LED::Update();
}

void FactoryTest::KeyEvent(uint16_t keyID, KeyInfo keyInfo)
{
    if(keyID == 0 && keyInfo.state == PRESSED)
    {
        current_test++;
        MatrixOS::LED::Fill(0);
    }
}

