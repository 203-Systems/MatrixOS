#pragma once

//Setting::Setup()
{
    AddUIElement(new UIButtonWithColorFunc("Bluetooth", 
            []() -> Color {return Color(0x0082fc).ToLowBrightness(Device::BLEMIDI::started);}, //TODO Color Class Scale Brightness
            []() -> void {
                Device::BLEMIDI::Toggle();
                Device::bluetooth = Device::BLEMIDI::started;
                }), 
            Point(0, 0));
}