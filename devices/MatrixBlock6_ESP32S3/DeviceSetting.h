#pragma once

// Setting::Setup()
UIButtonDimmable bluetoothToggle(
    "Bluetooth", Color(0x0082fc), []() -> bool { return Device::BLEMIDI::started; },
    []() -> void {
      Device::BLEMIDI::Toggle();
      Device::bluetooth = Device::BLEMIDI::started;
    });
AddUIComponent(bluetoothToggle, Point(0, 0));

UIButtonDimmable touchbarToggle(
    "Touchbar", Color(0x7957FB), []() -> bool { return Device::touchbar_enable; },
    []() -> void { Device::touchbar_enable = !Device::touchbar_enable; });
AddUIComponent(touchbarToggle, Point(0, 2));