#include "usb_desc_msc.h"
#include "../USB.h"
#include "MatrixOS.h"

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_msc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_MSC_TOTAL, 0, CONFIG_MSC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 5, EPNUM_MSC_OUT, EPNUM_MSC_IN, CFG_TUD_MSC_EP_BUFSIZE)
};

//--------------------------------------------------------------------+
// TinyUSB Callback Implementations
//--------------------------------------------------------------------+

static tusb_desc_device_t desc_device;

uint8_t const* msc_device_descriptor_cb(void) {
  desc_device = {.bLength = sizeof(tusb_desc_device_t),
                 .bDescriptorType = TUSB_DESC_DEVICE,
                 .bcdUSB = 0x0200,

                 // MSC uses device class 0 (per-interface class)
                 .bDeviceClass = 0,
                 .bDeviceSubClass = 0,
                 .bDeviceProtocol = 0,
                 .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

                 .idVendor = Device::usb_vid,
                 .idProduct = 0x0001,  // Different PID for Windows driver separation
                 .bcdDevice = (uint16_t)((MATRIXOS_MAJOR_VER << 8) + (MATRIXOS_MINOR_VER << 4) + (MATRIXOS_PATCH_VER)),

                 .iManufacturer = 0x01,
                 .iProduct = 0x02,
                 .iSerialNumber = 0x03,

                 .bNumConfigurations = 0x01};

  return (uint8_t const*)&desc_device;
}

uint8_t const* msc_configuration_descriptor_cb(uint8_t index) {
  (void)index;  // Ignore index like TinyUSB dynamic_configuration example
  return desc_msc_configuration;
}


static uint16_t _desc_str[32];

uint16_t const* msc_string_descriptor_cb(uint8_t index, uint16_t langid) {
  (void)langid;

  uint8_t chr_count;

  string product_name = Device::product_name;

  if (MatrixOS::UserVar::device_id.Get())
  {
    product_name += " ";
    product_name += std::to_string(MatrixOS::UserVar::device_id.Get());
  }

  // Add "MSC" suffix to distinguish MSC mode
  product_name += " MSC";

  string serial_number = Device::GetSerial();

  // array of pointer to string descriptors
  const char* string_desc_arr[] = {
      (const char[]){0x09, 0x04},              // 0: is supported language is English (0x0409)
      Device::manufacturer_name.c_str(),        // 1: Manufacturer
      product_name.c_str(),                    // 2: Product (with MSC suffix)
      serial_number.c_str(),                   // 3: Serials, should use chip ID
      "Reserved",                              // 4: Reserved (was CDC Interface)
      (Device::product_name + " Storage").c_str()  // 5: MSC Interface
  };

  if (index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }
  else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
      return NULL;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = strlen(str);
    if (chr_count > 31)
      chr_count = 31;

    // Convert ASCII string into UTF-16
    for (uint8_t i = 0; i < chr_count; i++)
    { _desc_str[1 + i] = str[i]; }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

  return _desc_str;
}