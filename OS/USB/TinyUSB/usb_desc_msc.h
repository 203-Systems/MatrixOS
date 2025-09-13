#pragma once

#include "tusb.h"

// MSC mode: Mass Storage only
enum MSC_INTERFACES
{
  ITF_NUM_MSC = 0,
  ITF_NUM_MSC_TOTAL
};

#define CONFIG_MSC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

#define EPNUM_MSC_OUT   0x01
#define EPNUM_MSC_IN    0x81

// TinyUSB callback function declarations
uint8_t const* msc_device_descriptor_cb(void);
uint8_t const* msc_configuration_descriptor_cb(uint8_t index);
uint16_t const* msc_string_descriptor_cb(uint8_t index, uint16_t langid);