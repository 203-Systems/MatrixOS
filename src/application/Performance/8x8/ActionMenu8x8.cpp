#include "ActionMenu8x8.h"

void ActionMenu::Setup()
{   
    //Brightness Control
    AddUIElement(UIElement("Brightness", Color(0xFFFFFF), [&]() -> void {NextBrightness();}), 4, Point(3, 3), Point(3, 4), Point(4, 3), Point(4, 4));

    //Rotation control and canvas
    AddUIElement(UIElement("Clear Canvas", Color(0x00FF00), [&]() -> void {MatrixOS::Logging::LogDebug(name, "Clear Canvas");}), 2, Point(3, 2), Point(4, 2));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), [&]() -> void {RotateClockwise(RIGHT);}), 2, Point(5, 3), Point(5, 4));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), [&]() -> void {RotateClockwise(DOWN);}), 2, Point(3, 5), Point(4, 5));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), [&]() -> void {RotateClockwise(LEFT);}), 2, Point(2, 3), Point(2, 4));

    // AddUIElement(UIElement("Bootloader", Color(0x00FFFF), [&]() -> void {MatrixOS::SYS::Bootloader();}), Point(2, 2));

    AddUIElement(UIElement("Bootloader", Color(0x00FFFF), [&]() -> void {MatrixOS::SYS::OpenSetting();}), Point(0, 7));

}

void ActionMenu::RotateClockwise(EDirection rotation)
{
    MatrixOS::Logging::LogDebug(name, "Rotate %d", (int)rotation);
}

void ActionMenu::NextBrightness()
{
    MatrixOS::Logging::LogDebug("Next Brightness", "Address of ActionMenu is %p", this);
    // uint8_t current_brightness = 16;
    // uint8_t current_brightness = (uint8_t)MatrixOS::SYS::GetVariable("brightnss");
    // MatrixOS::Logging::LogDebug(name, "Brightness Change %d", current_brightness);
    //  &current_brightness
    // for (uint8_t brightness: brightness_level)
    // {
    // //     // MatrixOS::Logging::LogDebug(name, "Check Brightness Level  %d", brightness);
    //     if (brightness > current_brightness)
    //     {
    // //         // MatrixOS::Logging::LogDebug(name, "Brightness Level Selected");
    //         MatrixOS::SYS::SetVariable("brightness", brightness);
    //         return;
    //     }
    // }
    // MatrixOS::Logging::LogDebug(name, "Lowest Level Selected");
    // MatrixOS::SYS::SetVariable("brightness", 16);
}

// void ActionMenu::KeyEvent(uint16_t KeyID, KeyInfo keyInfo)
// {
//     Point xy = MatrixOS::KEYPAD::ID2XY(KeyID);
//     if(xy) //IF XY is vaild, means it's on the main grid
//     {
//         MatrixOS::Logging::LogDebug(name, "Key Event %d %d", xy.x, xy.y);
//         if(xy.x == 0 && xy.y == 0)
//             NextBrightness();
//     }
//     else //XY Not vaild, 
//     {
//         // IDKeyEvent(KeyID, keyInfo);
//     }
// }