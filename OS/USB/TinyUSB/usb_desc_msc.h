#pragma once

#include "tusb.h"

// TinyUSB callback function declarations
uint8_t const* msc_device_descriptor_cb(void);
uint8_t const* msc_configuration_descriptor_cb(uint8_t index);
uint16_t const* msc_string_descriptor_cb(uint8_t index, uint16_t langid);