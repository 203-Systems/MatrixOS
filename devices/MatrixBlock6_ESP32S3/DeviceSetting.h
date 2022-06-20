#pragma once

//Setting::Setup()
{
    AddUIComponent(new UIButtonDimmable(
            "Bluetooth", 
            Color(0x0082fc),
            []() -> bool {return Device::BLEMIDI::started;},
            []() -> void {
                Device::BLEMIDI::Toggle();
                Device::bluetooth = Device::BLEMIDI::started;
                }), 
            Point(0, 0));

    AddUIComponent(new UIButtonDimmable(
        "Touchbar", 
        Color(0x7957FB),
        []() -> bool {return Device::touchbar_enable;},
        []() -> void {Device::touchbar_enable = !Device::touchbar_enable;}), 
        Point(0, 2));
}