#include "Setting.h"

void Setting::Setup()
{   
    //TODO: Let's assume all dimension are even atm. (No device with odd dimension should exist. Srsly why does Samson Conspiracy exists?)
    //Also assume at least 4x4

    name = "Setting";
    nameColor = Color(0x00FFFF);


    //Brightness Control
    AddUIElement(new UIButtonLarge("UIButtonLarge", Color(0xFFFFFF), Dimension(2,2), []() -> void {MatrixOS::SYS::NextBrightness();}), origin);

    //Rotation control and canvas
    AddUIElement(new UIButtonLarge("This does nothing", Color(0x00FF00), Dimension(2,1), []() -> void {}), origin + Point(0, -1));
    AddUIElement(new UIButtonLarge("Rotate to this side", Color(0x00FF00), Dimension(1,2), []() -> void {MatrixOS::SYS::Rotate(RIGHT);}), origin + Point(2, 0));
    AddUIElement(new UIButtonLarge("Rotate to this side", Color(0x00FF00), Dimension(2,1), []() -> void {MatrixOS::SYS::Rotate(DOWN);}), origin + Point(0, 2));
    AddUIElement(new UIButtonLarge("Rotate to this side", Color(0x00FF00), Dimension(1,2), []() -> void {MatrixOS::SYS::Rotate(LEFT);}), origin + Point(-1, 0));

    //Device Control
    AddUIElement(new UIButton("Enter DFU Mode", Color(0xFF0000), []() -> void {MatrixOS::SYS::Bootloader();}), Point(0, Device::y_size - 1));
    // AddUIElement(new UIButton("Clear Device Config", Color(0xFF00FF), []() -> void {}), Point(0, Device::y_size - 2));
    AddUIElement(new UIButton("Matrix OS Version", Color(0x00FF30), []() -> void {MatrixOS::UIComponent::TextScroll("Matrix OS " MATRIXOS_VERSION_STRING, Color(0x00FFFF));}), Point(1, Device::y_size - 1));
    AddUIElement(new UIButton("Device Name", Color(0x00FF30), []() -> void {MatrixOS::UIComponent::TextScroll(Device::name, Color(0x00FFFF));}), Point(2, Device::y_size - 1));

    // //Velocity Sensitive
    AddUIElement(new UIButtonDimmable("Velocity Sensitive", 
        Color(0xFFFFFF),
        []() -> bool {return MatrixOS::UserVar::velocity_sensitive_threshold.Get() == 0;}, //TODO Color Class Scale Brightness
        []() -> void {
            MatrixOS::UserVar::velocity_sensitive_threshold = MatrixOS::UserVar::velocity_sensitive_threshold.Get() ? Fract16(0) : Device::KeyPad::low_threshold; //TODO, allow user change threshold?
            }), 
        Point(7, 0));

    // AddUIElement(new UIButton("Color Correction", Color(0xFFFFFF), []() -> void {}), Point(6, 7));
    // AddUIElement(new UIButton("Device ID", Color(0x00FFAA), []() -> void {}), Point(Device::x_size - 1, Device::y_size - 1));

    #ifdef DEVICE_SETTING
    #include "DeviceSetting.h"
    #endif
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

                ab.AddUIElement(new UIButtonLarge("A", Color(0xFF0000), Dimension(2,2), [&]() -> void {if(konami == 9) MatrixOS::SYS::ExecuteAPP("203 Electronics", "REDACTED"); else ab.Exit();}), origin + Point(-1, 0));
                ab.AddUIElement(new UIButtonLarge("B", Color(0xFF0000), Dimension(2,2), [&]() -> void {if(konami == 8) konami++; else ab.Exit();}), origin + Point(2, 0));

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
