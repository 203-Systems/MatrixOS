#include "MatrixOS.h"

#define NKRO_COUNT 6 // Only 6 is supported by the current USB stack

namespace MatrixOS::HID
{
  bool Ready()
  {
    return tud_hid_ready();
  }

  vector<uint8_t> activeKeycode;


  // TODO: Current implentment has a know issue
  // If the key is pressed when HID isn't ready
  // That key won't be sent untill the next keypress (They will be batched together)
  bool KeyboardPress(uint8_t keycode) 
  {
    activeKeycode.reserve(6);
    // MLOGD("HID", "Send keypress %d", keycode);

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
    activeKeycode.resize(NKRO_COUNT); //Pad out the data() array with 0s
    activeKeycode.resize(new_size); //Shrink back to the correct size

    if(!Ready()) {
    // MLOGD("HID", "HID not ready");
    return false; }

    if (tud_suspended())
    {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }

    uint8_t const modifier = 0;
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, activeKeycode.data());
      // MLOGD("HID", "Keypress %d sent - %d %d %d %d %d %d", keycode, activeKeycode[0], activeKeycode[1], activeKeycode[2], activeKeycode[3], activeKeycode[4], activeKeycode[5]);
    return true;
  }

  void KeyboardRelease(uint8_t keycode) 
  {
    // MLOGD("HID", "Release keypress %d", keycode);
    
    // Check if keycode is in activeKeycode
    for (uint8_t i = 0; i < activeKeycode.size(); i++)
    {
      if (activeKeycode[i] == keycode)
      {
        activeKeycode.erase(activeKeycode.begin() + i);
        uint8_t new_size = activeKeycode.size();
        activeKeycode.resize(NKRO_COUNT); //Pad out the data() array with 0s
        activeKeycode.resize(new_size); //Shrink back to the correct size
        break;
      }
    }

    if(!Ready()) {
      // MLOGD("HID", "HID not ready");
      return;
    }

    uint8_t const modifier  = 0;
    if(!activeKeycode.empty())
    { tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, activeKeycode.data()); }
    else
    { tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, NULL); }

    // MLOGD("HID", "Keypress %d released - %d %d %d %d %d %d", keycode, activeKeycode[0], activeKeycode[1], activeKeycode[2], activeKeycode[3], activeKeycode[4], activeKeycode[5]);
    
  }
}