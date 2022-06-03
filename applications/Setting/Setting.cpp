#include "Setting.h"

void Setting::Setup()
{   
    //TODO: Let's assume all dimension are even atm. (No device with odd dimension should exist. Srsly why does Samson Conspiracy exists?)
    //Also assume at least 4x4

    //Brightness Control
    AddUIElement(UIElement("Brightness", Color(0xFFFFFF), []() -> void {MatrixOS::SYS::NextBrightness();}), 4, origin, origin + Point(0, 1), origin + Point(1, 0), origin + Point(1, 1));

    //Rotation control and canvas
    AddUIElement(UIElement("This does nothing", Color(0x00FF00), []() -> void {}), 2, origin + Point(0, -1), origin + Point(1, -1));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), []() -> void {MatrixOS::SYS::Rotate(RIGHT);}), 2, origin + Point(2, 0), origin + Point(2, 1));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), []() -> void {MatrixOS::SYS::Rotate(DOWN);}), 2, origin + Point(0, 2), origin + Point(1, 2));
    AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), []() -> void {MatrixOS::SYS::Rotate(LEFT);}), 2, origin + Point(-1, 0), origin + Point(-1, 1));

    //Device Control
    AddUIElement(UIElement("Enter DFU Mode", Color(0xFF0000), []() -> void {MatrixOS::SYS::Bootloader();}), Point(0, Device::y_size - 1));
    // AddUIElement(UIElement("Clear Device Config", Color(0xFF00FF), []() -> void {}), Point(0, Device::y_size - 2));
    AddUIElement(UIElement("Matrix OS Version", Color(0x00FF30), []() -> void {MatrixOS::UIComponent::TextScroll("Matrix OS " MATRIXOS_VERSION_STRING, Color(0x00FFFF));}), Point(1, Device::y_size - 1));
    AddUIElement(UIElement("Device Name", Color(0x00FF30), []() -> void {MatrixOS::UIComponent::TextScroll(Device::name, Color(0x00FFFF));}), Point(2, Device::y_size - 1));

    // //Velocity Sensitive
    // AddUIElement(UIElement("Velocity Sensitive", Color(0xFFFFFF), []() -> void {MatrixOS::SYS::NextBrightness();}), Point(0, 0));



    // AddUIElement(UIElement("Color Correction", Color(0xFFFFFF), []() -> void {}), Point(6, 7));
    // AddUIElement(UIElement("Device ID", Color(0x00FFAA), []() -> void {}), Point(Device::x_size - 1, Device::y_size - 1));
}

bool Setting::KeyEvent(uint16_t KeyID, KeyInfo keyInfo)
{
    Point xy = MatrixOS::KEYPAD::ID2XY(KeyID);
    if(xy && keyInfo.state == RELEASED) //IF XY is vaild, means it's on the main grid
    {
        if((konami == 0 || konami == 1) && (xy == origin + Point(0, -1) || xy == origin + Point(1, -1)))
        {
            konami ++;
            MatrixOS::Logging::LogDebug("Konami", "Up prssed, %d", konami);
            return false;
        }
        else if((konami == 2 || konami == 3) && (xy == origin + Point(0, 2) || xy == origin + Point(1, 2)))
        {
            konami ++;
            MatrixOS::Logging::LogDebug("Konami", "Down prssed, %d", konami);
            return true;
        }
        else if((konami == 4 || konami == 6) && (xy == origin + Point(-1, 0) || xy == origin + Point(-1, 1)))
        {
            konami ++;
            MatrixOS::Logging::LogDebug("Konami", "Left prssed, %d", konami);
            return true;
        }
        else if((konami == 5 || konami == 7) && (xy == origin + Point(2, 0) || xy == origin + Point(1, 2)))
        {
            konami ++;
            MatrixOS::Logging::LogDebug("Konami", "Right prssed, %d", konami);
            if(konami == 8)
            {
                UI ab("A & B",  Color(0xFF0000));

                ab.AddUIElement(UIElement("A", Color(0xFF0000), [&]() -> void {if(konami == 9) MatrixOS::SYS::ExecuteAPP("203 Electronics", "REDACTED"); else ab.Exit();}), 4, origin + Point(-1, 0), origin + Point(-1, 1), origin + Point(-2, 0), origin + Point(-2, 1));
                ab.AddUIElement(UIElement("B", Color(0xFF0000), [&]() -> void {if(konami == 8) konami++; else ab.Exit();}), 4, origin + Point(2, 0), origin + Point(2, 1), origin + Point(3, 0), origin + Point(3, 1));

                ab.Start();
            }
            return true;
        }
        else
        {
            MatrixOS::Logging::LogDebug("Konami", "Cleared");
            konami = 0;
            return false;
        }
    }
    return false;
}
