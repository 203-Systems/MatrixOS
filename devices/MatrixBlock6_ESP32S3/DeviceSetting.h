#pragma once

//Setting::Setup()
{
    AddUIElement(new UIButtonDimmable(
            "Bluetooth", 
            Color(0x0082fc),
            []() -> bool {return Device::BLEMIDI::started;},
            []() -> void {
                Device::BLEMIDI::Toggle();
                Device::bluetooth = Device::BLEMIDI::started;
                }), 
            Point(0, 0));
}