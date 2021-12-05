#include "Setting.h"

void Setting::Setup()
{   
    //Brightness Control
    AddUIElement(UIElement("Brightness", Color(0xFFFFFF), []() -> void {NextBrightness();}), 4, Point(3, 3), Point(3, 4), Point(4, 3), Point(4, 4));

    //Rotation control and canvas
    AddUIElement(UIElement("Clear Canvas", Color(0x00FF00), []() -> void {MatrixOS::USB::CDC::Println("Clear Canvas");}), 2, Point(3, 2), Point(4, 2));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), []() -> void {RotateClockwise(RIGHT);}), 2, Point(5, 3), Point(5, 4));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), []() -> void {RotateClockwise(DOWN);}), 2, Point(3, 5), Point(4, 5));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), []() -> void {RotateClockwise(LEFT);}), 2, Point(2, 3), Point(2, 4));

    //Device Control
    AddUIElement(UIElement("Enter DFU Mode", Color(0xFF0000), []() -> void {MatrixOS::SYS::Bootloader();}), Point(0, 7));
    AddUIElement(UIElement("Clear Device Config", Color(0xFF00FF), []() -> void {}), Point(0, 6));
    AddUIElement(UIElement("Matrix OS Version", Color(0x00FF30), []() -> void {}), Point(1, 7));
    AddUIElement(UIElement("Device Bootloader Version", Color(0x00FF30), []() -> void {}), Point(2, 7));
    AddUIElement(UIElement("Device Name", Color(0x00FF30), []() -> void {}), Point(3, 7));


    AddUIElement(UIElement("Color Correction", Color(0xFFFFFF), []() -> void {}), Point(6, 7));
    AddUIElement(UIElement("Device ID", Color(0x00FFAA), []() -> void {}), Point(7, 7));
}

void Setting::RotateClockwise(EDirection rotation)
{
    
}

void Setting::NextBrightness()
{
    uint8_t current_brightness = (uint8_t)MatrixOS::SYS::GetVariable("brightness");
    MatrixOS::USB::CDC::Print("Brightness: ");
    MatrixOS::USB::CDC::Println(std::to_string(current_brightness).c_str());
    for (uint8_t i = 0; i < sizeof(brightness_level); i++)
    {
        if (brightness_level[i] > current_brightness)
        {
            MatrixOS::SYS::SetVariable("brightness", brightness_level[i]);
            return;
        }
    }
    MatrixOS::SYS::SetVariable("brightness", brightness_level[0]);
}

void Setting::KeyEvent(uint16_t KeyID, KeyInfo keyInfo)
{
    
}

