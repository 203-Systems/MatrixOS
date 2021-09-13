#include "ActionMenu8x8.h"

void ActionMenu::Setup()
{   
    //Brightness Control
    AddUIElement(UIElement("Brightness", Color(0xFFFFFF), []() -> void {NextBrightness();}), 4, Point(3, 3), Point(3, 4), Point(4, 3), Point(4, 4));

    //Rotation control and canvas
    AddUIElement(UIElement("Clear Canvas", Color(0x00FF00), []() -> void {MatrixOS::USB::CDC::Println("Clear Canvas");}), 2, Point(3, 2), Point(4, 2));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), []() -> void {RotateClockwise(RIGHT);}), 2, Point(5, 3), Point(5, 4));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), []() -> void {RotateClockwise(DOWN);}), 2, Point(3, 5), Point(4, 5));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), []() -> void {RotateClockwise(LEFT);}), 2, Point(2, 3), Point(2, 4));

}

void ActionMenu::RotateClockwise(EDirection rotation)
{
    MatrixOS::USB::CDC::Println("Rotate");
}

void ActionMenu::NextBrightness()
{
    uint8_t current_brightness = (uint8_t)MatrixOS::SYS::GetVariable(MatrixOS::SYS::ESysVar::Brightness);
    MatrixOS::USB::CDC::Print("Brightness: ");
    MatrixOS::USB::CDC::Println(std::to_string(current_brightness).c_str());
    for (uint8_t i = 0; i < sizeof(brightness_level); i++)
    {
        if (brightness_level[i] > current_brightness)
        {
            MatrixOS::SYS::SetVariable(MatrixOS::SYS::ESysVar::Brightness, brightness_level[i]);
            return;
        }
    }
    MatrixOS::SYS::SetVariable(MatrixOS::SYS::ESysVar::Brightness, brightness_level[0]);
}

void ActionMenu::KeyEvent(uint16_t KeyID, KeyInfo keyInfo)
{
    // MatrixOS::USB::CDC::Println("KeyEvent");
}

