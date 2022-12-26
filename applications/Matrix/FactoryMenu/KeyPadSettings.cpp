#include "FactoryMenu.h"
void FactoryMenu::KeyPadSettings() {
  UI keypadSettings("Keypad Settings", Color(0x00FFFF));

  bool need_reboot = false;

  UIButtonDimmable customKeypadSettingToggle(
      "Custom Keypad Setting", Color(0x00FFFF), [&]() -> bool { return Device::KeyPad::keypad_custom_setting; },
      [&]() -> void {
        Device::KeyPad::keypad_custom_setting = !Device::KeyPad::keypad_custom_setting;
        need_reboot = true;
      });
  keypadSettings.AddUIComponent(customKeypadSettingToggle, Point(0, 0));

  UIButtonWithColorFunc keypadVelocitySensitiveToggle(
      "velocity_sensitive",
      [&]() -> Color { return Color(0xFF00FF).ToLowBrightness(Device::KeyPad::keypad_velocity_sensitive); },
      [&]() -> void {
        if (Device::KeyPad::keypad_custom_setting)
        {
          Device::KeyPad::keypad_velocity_sensitive = !Device::KeyPad::keypad_velocity_sensitive;
          need_reboot = true;
        }
      });
  keypadSettings.AddUIComponent(keypadVelocitySensitiveToggle, Point(1, 0));

  UIButtonWithColorFunc keypadLowThresholdCtrl(
      "Keypad Low Threshold",
      [&]() -> Color { return Device::KeyPad::keypad_custom_setting && Device::KeyPad::keypad_velocity_sensitive ? Color(0xFFFFFF) : Color(0x400000); },
      [&]() -> void {
        if (Device::KeyPad::keypad_custom_setting && Device::KeyPad::keypad_velocity_sensitive)
        {
          Device::KeyPad::keypad_low_threshold =
              (Fract16)(MatrixOS::UIInterface::NumberSelector8x8((uint16_t)Device::KeyPad::keypad_low_threshold.Get() /
                                                                     256,
                                                                 0x00FFFF, "Keypad Low Threshold", 1, 16) *
                        256);
          Device::KeyPad::keypad_config.low_threshold = Device::KeyPad::keypad_low_threshold;
        }
      });
  keypadSettings.AddUIComponent(keypadLowThresholdCtrl, Point(2, 0));

  UIButtonWithColorFunc keypadHighThresholdCtrl(
      "Keypad High Threshold",
      [&]() -> Color { return Device::KeyPad::keypad_custom_setting && Device::KeyPad::keypad_velocity_sensitive ? Color(0xFFFFFF) : Color(0x400000); },
      [&]() -> void {
        if (Device::KeyPad::keypad_custom_setting && Device::KeyPad::keypad_velocity_sensitive)
        {
          Device::KeyPad::keypad_high_threshold =
              (Fract16)(MatrixOS::UIInterface::NumberSelector8x8((uint16_t)Device::KeyPad::keypad_high_threshold.Get() /
                                                                     256,
                                                                 0x00FFFF, "Keypad High Threshold",
                                                                 1, 255) *
                        256);
          Device::KeyPad::keypad_config.high_threshold = Device::KeyPad::keypad_high_threshold;
        }
      });
  keypadSettings.AddUIComponent(keypadHighThresholdCtrl, Point(3, 0));

  UIButtonWithColorFunc keypadDebounceCtrl(
      "Keypad Debounce (ms)",
      [&]() -> Color { return Device::KeyPad::keypad_custom_setting ? Color(0xFFFFFF) : Color(0x000000); },
      [&]() -> void {
        if (Device::KeyPad::keypad_custom_setting)
        {
          Device::KeyPad::keypad_debounce =
              MatrixOS::UIInterface::NumberSelector8x8((uint16_t)Device::KeyPad::keypad_debounce.Get(), 0x00FFFF,
                                                       "Keypad Debounce (ms)", 0, 32);
          Device::KeyPad::keypad_config.debounce = Device::KeyPad::keypad_debounce;
        }
      });
  keypadSettings.AddUIComponent(keypadDebounceCtrl, Point(4, 0));

  UIButtonWithColorFunc rebootIndicator(
      "Reboot Needed", [&]() -> Color { return need_reboot ? Color(0xFF0000) : Color(0x000000); },
      [&]() -> void {
        if (need_reboot)
        { MatrixOS::SYS::Reboot(); }
      });
  keypadSettings.AddUIComponent(rebootIndicator, Point(7, 0));

  keypadSettings.Start();

  if (need_reboot)
  {
    MatrixOS::SYS::DelayMs(500);  // Prevent reboot into bootloader
    MatrixOS::SYS::Reboot();
  }
}