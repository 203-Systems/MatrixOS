#pragma once

#include "tusb.h"

// USB mode interface structure for dynamic configuration
// Each USB mode implements all required TinyUSB descriptor callbacks
struct usb_mode_interface {
  // TinyUSB descriptor callbacks
  uint8_t const* (*get_device_descriptor)(void);
  uint8_t const* (*get_configuration_descriptor)(uint8_t index);
  uint16_t const* (*get_string_descriptor)(uint8_t index, uint16_t langid);
  uint8_t const* (*get_hid_report_descriptor)(uint8_t instance);  // NULL for modes without HID
};

// Mode table declaration (defined in usb_descriptors.cpp)
extern const struct usb_mode_interface mode_table[];