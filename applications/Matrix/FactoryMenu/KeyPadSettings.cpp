#include "FactoryMenu.h"
void FactoryMenu::KeyPadSettings()
{
    UI keypadSettings("Keypad Settings", Color(0x00FFFF));

    bool need_reboot = false;

    keypadSettings.AddUIComponent(new UIButtonDimmable(
        "Custom Keypad Setting", 
        Color(0x00FFFF),
        [&]() -> bool {return Device::KeyPad::keypad_custom_setting;},
        [&]() -> void {Device::KeyPad::keypad_custom_setting = !Device::KeyPad::keypad_custom_setting; need_reboot = true;}), 
        Point(0, 0));

    keypadSettings.AddUIComponent(new UIButtonWithColorFunc(
        "Keypad Low Threshold", 
        [&]() -> Color {return Device::KeyPad::keypad_custom_setting ? Color(0xFFFFFF) : Color(0x000000);},
        [&]() -> void {if(Device::KeyPad::keypad_custom_setting) {Device::KeyPad::keypad_low_threshold = (Fract16)(MatrixOS::UIInterface::NumberSelector8x8((uint16_t)Device::KeyPad::keypad_low_threshold.Get() / 256, 0x00FFFF, "Keypad Low Threshold", 0, 16) * 256);}}), 
        Point(1, 0));

    keypadSettings.AddUIComponent(new UIButtonWithColorFunc(
        "Keypad High Threshold", 
        [&]() -> Color {return Device::KeyPad::keypad_custom_setting ? Color(0xFFFFFF) : Color(0x000000);},
        [&]() -> void {if(Device::KeyPad::keypad_custom_setting) {Device::KeyPad::keypad_high_threshold = (Fract16)(MatrixOS::UIInterface::NumberSelector8x8((uint16_t)Device::KeyPad::keypad_high_threshold.Get() / 256, 0x00FFFF, "Keypad High Threshold", Device::KeyPad::keypad_low_threshold.Get(), 255) * 256);}}), 
        Point(2, 0));

    keypadSettings.AddUIComponent(new UIButtonWithColorFunc(
        "Keypad Debounce (ms)", 
        [&]() -> Color {return Device::KeyPad::keypad_custom_setting ? Color(0xFFFFFF) : Color(0x000000);},
        [&]() -> void {if(Device::KeyPad::keypad_custom_setting) {Device::KeyPad::keypad_debounce = MatrixOS::UIInterface::NumberSelector8x8((uint16_t)Device::KeyPad::keypad_debounce.Get(), 0x00FFFF, "Keypad Debounce (ms)", 0, 255);}}), 
        Point(3, 0));

    keypadSettings.AddUIComponent(new UIButtonWithColorFunc(
        "Reboot Needed", 
        [&]() -> Color {return need_reboot ? Color(0xFF0000) : Color(0x000000);},
        [&]() -> void {if(need_reboot){MatrixOS::SYS::Reboot();}}), 
        Point(7, 0));


    keypadSettings.Start();

    if(need_reboot)
    {
        MatrixOS::SYS::DelayMs(500); //Prevent reboot into bootloader
        MatrixOS::SYS::Reboot();
    }
}