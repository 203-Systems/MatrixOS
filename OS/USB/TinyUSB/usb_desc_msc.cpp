#include "usb_desc_msc.h"
#include "../USB.h"
#include "MatrixOS.h"

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

// MSC mode: CDC + Mass Storage composite
enum MSC_INTERFACES
{
  ITF_NUM_MSC,
  ITF_NUM_TOTAL
};

#define CONFIG_MSC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

#define EPNUM_MSC_OUT       0x01
#define EPNUM_MSC_IN        0x81


uint8_t const desc_msc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_MSC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 4, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64)

};

//--------------------------------------------------------------------+
// TinyUSB Callback Implementations
//--------------------------------------------------------------------+

static tusb_desc_device_t desc_device;

uint8_t const* msc_device_descriptor_cb(void) {
  desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = Device::usb_vid,
    .idProduct = 0x0001,
    .bcdDevice = MATRIXOS_VERSION_ID_16,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01
  };

  return (uint8_t const*)&desc_device;
}

uint8_t const* msc_configuration_descriptor_cb(uint8_t index) {
  (void)index;  // Ignore index like TinyUSB dynamic_configuration example
  return desc_msc_configuration;
}


// One header word plus the complete 32-character device serial.
static uint16_t _desc_str[33];

uint16_t const* msc_string_descriptor_cb(uint8_t index, uint16_t langid) {
  (void)langid;

  size_t chr_count;
  const string serial = Device::GetSerial();
  const string interfaceName = Device::productName + " MSC";

  // array of pointer to string descriptors
  const char* string_desc_arr[] = {
      nullptr,                                // 0: language descriptor is handled below
      Device::manufacturerName.c_str(),        // 1: Manufacturer
      Device::productName.c_str(),                    // 2: Product (with MSC suffix)
      serial.c_str(),                         // 3: Serial
      interfaceName.c_str()                   // 4: MSC Interface
  };

  if (index == 0)
  {
    _desc_str[1] = 0x0409; // English (United States)
    chr_count = 1;
  }
  else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
      return nullptr;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = strlen(str);
    const size_t maxChars = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;
    if (chr_count > maxChars)
      chr_count = maxChars;

    // Convert ASCII string into UTF-16
    for (size_t i = 0; i < chr_count; i++)
    { _desc_str[1 + i] = static_cast<uint8_t>(str[i]); }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

  return _desc_str;
}
