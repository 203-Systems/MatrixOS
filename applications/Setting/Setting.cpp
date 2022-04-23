#include "Setting.h"

void Setting::Setup()
{   

    Point origin = Point((Device::x_size - 1)/2, (Device::y_size - 1)/2);

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
    AddUIElement(UIElement("Clear Device Config", Color(0xFF00FF), []() -> void {}), Point(0, Device::y_size - 2));
    AddUIElement(UIElement("Matrix OS Version", Color(0x00FF30), []() -> void {}), Point(1, Device::y_size - 1));
    AddUIElement(UIElement("Device Bootloader Version", Color(0x00FF30), []() -> void {}), Point(2, Device::y_size - 1));
    AddUIElement(UIElement("Device Name", Color(0x00FF30), []() -> void {}), Point(3, Device::y_size - 1));


    // AddUIElement(UIElement("Color Correction", Color(0xFFFFFF), []() -> void {}), Point(6, 7));
    AddUIElement(UIElement("Device ID", Color(0x00FFAA), []() -> void {}), Point(Device::x_size - 1, Device::y_size - 1));

    // Device::Setting::Setup();

    AddUIElement(UIElement("Test", Color(0xFFFFFF), []() -> void {MatrixOS::UIComponent::TextScroll("Hello World", Color(0xFFFFFF));}), Point(Device::x_size - 2, Device::y_size - 1));
}


