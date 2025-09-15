#include "../USB.h"
#include "MatrixOS.h"
#include "tusb.h"

#include "usb_mode_interface.h"

#include "usb_desc_default.h"
#include "usb_desc_msc.h"

//--------------------------------------------------------------------+
// Mode Dispatch Table
//--------------------------------------------------------------------+

const struct usb_mode_interface mode_table[] = {
  [USB_MODE_NORMAL] = {
    .get_device_descriptor = default_device_descriptor_cb,
    .get_configuration_descriptor = default_configuration_descriptor_cb,
    .get_string_descriptor = default_string_descriptor_cb,
    .get_hid_report_descriptor = default_hid_report_descriptor_cb
  },
  [USB_MODE_MSC] = {
    .get_device_descriptor = msc_device_descriptor_cb,
    .get_configuration_descriptor = msc_configuration_descriptor_cb,
    .get_string_descriptor = msc_string_descriptor_cb,
    .get_hid_report_descriptor = NULL  // MSC mode has no HID
  }
};

//--------------------------------------------------------------------+
// TinyUSB Callback Dispatchers
//--------------------------------------------------------------------+
// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void) {
  uint8_t mode = MatrixOS::USB::GetMode();
  return mode_table[mode].get_device_descriptor();
}

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
  uint8_t mode = MatrixOS::USB::GetMode();
  if (mode_table[mode].get_hid_report_descriptor) {
    return mode_table[mode].get_hid_report_descriptor(instance);
  }
  return NULL;  // No HID in this mode
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  uint8_t mode = MatrixOS::USB::GetMode();
  return mode_table[mode].get_configuration_descriptor(index);
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  uint8_t mode = MatrixOS::USB::GetMode();
  return mode_table[mode].get_string_descriptor(index, langid);
}