#pragma once

#include "tusb.h"

// TinyUSB callback function declarations
uint8_t const* default_device_descriptor_cb(void);
uint8_t const* default_configuration_descriptor_cb(uint8_t index);
uint16_t const* default_string_descriptor_cb(uint8_t index, uint16_t langid);
uint8_t const* default_hid_report_descriptor_cb(uint8_t instance);