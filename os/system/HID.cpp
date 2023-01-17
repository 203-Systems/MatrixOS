#include "MatrixOS.h"

#define NKRO_COUNT 6

namespace MatrixOS::HID
{
  bool Ready()
  {
    return tud_hid_ready();
  }

  vector<uint8_t> activeKeycode;

  bool KeyboardPress(uint8_t keycode) 
  {
    activeKeycode.reserve(6);
    // MatrixOS::Logging::LogDebug("HID", "Send keypress %d", keycode);

    if (activeKeycode.size() >= NKRO_COUNT)
    { return false; }

    // Check if keycode is in activeKeycode
    for (uint8_t i = 0; i < activeKeycode.size(); i++)
    {
      if (activeKeycode[i] == keycode)
      {
        activeKeycode.erase(activeKeycode.begin() + i);
        break;
      }
    }

    activeKeycode.push_back(keycode);
    
    uint8_t new_size = activeKeycode.size();
    activeKeycode.resize(6); //Pad out the data() array with 0s
    activeKeycode.resize(new_size); //Shrink back to the correct size

    if(!Ready()) {
    // MatrixOS::Logging::LogDebug("HID", "HID not ready");
    return false; }

    if (tud_suspended())
    {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }

    uint8_t const modifier = 0;
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, activeKeycode.data());
      // MatrixOS::Logging::LogDebug("HID", "Keypress %d sent - %d %d %d %d %d %d", keycode, activeKeycode[0], activeKeycode[1], activeKeycode[2], activeKeycode[3], activeKeycode[4], activeKeycode[5]);
    return true;
  }

  void KeyboardRelease(uint8_t keycode) 
  {
    // MatrixOS::Logging::LogDebug("HID", "Release keypress %d", keycode);
    
    // Check if keycode is in activeKeycode
    for (uint8_t i = 0; i < activeKeycode.size(); i++)
    {
      if (activeKeycode[i] == keycode)
      {
        activeKeycode.erase(activeKeycode.begin() + i);
        uint8_t new_size = activeKeycode.size();
        activeKeycode.resize(6); //Pad out the data() array with 0s
        activeKeycode.resize(new_size); //Shrink back to the correct size
        break;
      }
    }

    if(!Ready()) {
      // MatrixOS::Logging::LogDebug("HID", "HID not ready");
      return;
    }

    uint8_t const modifier  = 0;
    if(!activeKeycode.empty())
    { tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, activeKeycode.data()); }
    else
    { tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, NULL); }

    // MatrixOS::Logging::LogDebug("HID", "Keypress %d released - %d %d %d %d %d %d", keycode, activeKeycode[0], activeKeycode[1], activeKeycode[2], activeKeycode[3], activeKeycode[4], activeKeycode[5]);
    
  }
}