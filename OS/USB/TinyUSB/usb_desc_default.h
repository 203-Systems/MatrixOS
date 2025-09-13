#pragma once

#include "tusb.h"

// Default mode: MIDI + CDC + HID
enum DEFAULT_INTERFACES
{
  ITF_NUM_MIDI = 0,
  ITF_NUM_MIDI_STREAMING,
  ITF_NUM_CDC,
  ITF_NUM_CDC_DATA,
  ITF_NUM_HID,
  ITF_NUM_DEFAULT_TOTAL
};

#define CONFIG_DEFAULT_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_DUO_MIDI_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

#define EPNUM_DEFAULT_MIDI_OUT  0x01
#define EPNUM_DEFAULT_MIDI_IN   0x81
#define EPNUM_DEFAULT_CDC_NOTIF 0x82
#define EPNUM_DEFAULT_CDC_OUT   0x02
#define EPNUM_DEFAULT_CDC_IN    0x83
#define EPNUM_DEFAULT_HID_OUT   0x03
#define EPNUM_DEFAULT_HID_IN    0x84

// TinyUSB callback function declarations
uint8_t const* default_device_descriptor_cb(void);
uint8_t const* default_configuration_descriptor_cb(uint8_t index);
uint16_t const* default_string_descriptor_cb(uint8_t index, uint16_t langid);
uint8_t const* default_hid_report_descriptor_cb(uint8_t instance);