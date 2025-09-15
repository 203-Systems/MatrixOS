#include "usb_desc_default.h"
#include "../USB.h"
#include "MatrixOS.h"

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

#define EXPAND(x) x

// Gamepad Report Descriptor Template
// with 32 buttons, 2 joysticks and 1 hat/dpad with following layout
// | X | Y | Z | Rz | Rx | Ry (2 byte each) | hat/DPAD (1 byte) | Button Map (4 bytes) |
#define HID_REPORT_DESC_GAMEPAD(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                 ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD  )                 ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION )                 ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    /* 16 bit X, Y, Z, Rz, Rx, Ry (min -32767, max 32767 ) */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_X                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Y                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_Z                    ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RZ                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RX                   ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_RY                   ) ,\
    HID_LOGICAL_MIN_N  ( 0x8001, 2                              ) ,\
    HID_LOGICAL_MAX_N  ( 0x7FFF, 2                              ) ,\
    HID_REPORT_COUNT   ( 6                                      ) ,\
    HID_REPORT_SIZE    ( 16                                     ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 8 bit DPad/Hat Button Map  */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_DESKTOP                 ) ,\
    HID_USAGE          ( HID_USAGE_DESKTOP_HAT_SWITCH           ) ,\
    HID_LOGICAL_MIN    ( 1                                      ) ,\
    HID_LOGICAL_MAX    ( 8                                      ) ,\
    HID_PHYSICAL_MIN   ( 0                                      ) ,\
    HID_PHYSICAL_MAX_N ( 315, 2                                 ) ,\
    HID_REPORT_COUNT   ( 1                                      ) ,\
    HID_REPORT_SIZE    ( 8                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
    /* 32 bit Button Map */ \
    HID_USAGE_PAGE     ( HID_USAGE_PAGE_BUTTON                  ) ,\
    HID_USAGE_MIN      ( 1                                      ) ,\
    HID_USAGE_MAX      ( 32                                     ) ,\
    HID_LOGICAL_MIN    ( 0                                      ) ,\
    HID_LOGICAL_MAX    ( 1                                      ) ,\
    HID_REPORT_COUNT   ( 32                                     ) ,\
    HID_REPORT_SIZE    ( 1                                      ) ,\
    HID_INPUT          ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
  HID_COLLECTION_END \

#define HID_REPORT_DESC_VENDOR() \
    HID_USAGE_PAGE_N ( HID_USAGE_PAGE_VENDOR, 2   ),\
    HID_USAGE        ( 0x01                       ),\
    HID_COLLECTION   ( HID_COLLECTION_APPLICATION ),\
      /* Report ID for APP command */\
      0x85, 255, \
      /* Input Report (64 bytes) */ \
      HID_USAGE   ( 0x10                                    ),\
      HID_LOGICAL_MIN ( 0                                       ),\
      HID_LOGICAL_MAX ( 255                                     ),\
      HID_REPORT_SIZE ( 8                                       ),\
      HID_REPORT_COUNT( 32                                      ),\
      HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ),\
      /* Output Report (64 bytes) */ \
      HID_USAGE   ( 0x10                                    ),\
      HID_LOGICAL_MIN ( 0                                       ),\
      HID_LOGICAL_MAX ( 255                                     ),\
      HID_REPORT_SIZE ( 8                                       ),\
      HID_REPORT_COUNT( 32                                      ),\
      HID_OUTPUT      ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ),\
    HID_COLLECTION_END,\
    HID_USAGE_PAGE_N ( HID_USAGE_PAGE_VENDOR, 2   ),\
    HID_USAGE        ( 0x01                       ),\
    HID_COLLECTION   ( HID_COLLECTION_APPLICATION ),\
      /* Report ID for System command */\
      0x85, 0xCB, \
      /* Input Report (64 bytes) */ \
      HID_USAGE   ( 0x10                                    ),\
      HID_LOGICAL_MIN ( 0                                       ),\
      HID_LOGICAL_MAX ( 255                                     ),\
      HID_REPORT_SIZE ( 8                                       ),\
      HID_REPORT_COUNT( 32                                      ),\
      HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ),\
      /* Output Report (64 bytes) */ \
      HID_USAGE   ( 0x10                                    ),\
      HID_LOGICAL_MIN ( 0                                       ),\
      HID_LOGICAL_MAX ( 255                                     ),\
      HID_REPORT_SIZE ( 8                                       ),\
      HID_REPORT_COUNT( 32                                      ),\
      HID_OUTPUT      ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ),\
    HID_COLLECTION_END,\

uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
                                   TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
                                   TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL)),
                                   HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(REPORT_ID_GAMEPAD)),
                                   HID_REPORT_DESC_VENDOR()
                                  };

//--------------------------------------------------------------------+
// MIDI Descriptor
//--------------------------------------------------------------------+
#define TUD_DUO_MIDI_DESC_LEN (TUD_MIDI_DESC_HEAD_LEN + TUD_MIDI_DESC_JACK_LEN * 2 + TUD_MIDI_DESC_EP_LEN(2) * 2)
#define TUD_DUO_MIDI_DESCRIPTOR(_itfnum, _stridx, _epout, _epin, _epsize) \
  TUD_MIDI_DESC_HEAD(_itfnum, _stridx, 2),\
  TUD_MIDI_DESC_JACK_DESC(1, 0),\
  TUD_MIDI_DESC_JACK_DESC(2, 0),\
  TUD_MIDI_DESC_EP(_epout, _epsize, 2),\
  TUD_MIDI_JACKID_IN_EMB(1),\
  TUD_MIDI_JACKID_IN_EMB(2),\
  TUD_MIDI_DESC_EP(_epin, _epsize, 2),\
  TUD_MIDI_JACKID_OUT_EMB(1),\
  TUD_MIDI_JACKID_OUT_EMB(2)

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

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
#define EPNUM_DEFAULT_CDC_OUT   0x03
#define EPNUM_DEFAULT_CDC_IN    0x83
#define EPNUM_DEFAULT_HID_OUT   0x04
#define EPNUM_DEFAULT_HID_IN    0x84


uint8_t const desc_default_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_DEFAULT_TOTAL, 0, CONFIG_DEFAULT_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_DUO_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 0, EPNUM_DEFAULT_MIDI_OUT, EPNUM_DEFAULT_MIDI_IN, CFG_TUD_MIDI_RX_BUFSIZE),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_DEFAULT_CDC_NOTIF, 8, EPNUM_DEFAULT_CDC_OUT, EPNUM_DEFAULT_CDC_IN, CFG_TUD_CDC_RX_BUFSIZE),

    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_DEFAULT_HID_OUT, EPNUM_DEFAULT_HID_IN, 64, 5)
};

//--------------------------------------------------------------------+
// TinyUSB Callback Implementations
//--------------------------------------------------------------------+

static tusb_desc_device_t desc_device;

uint8_t const* default_device_descriptor_cb(void) {
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
    .idProduct = Device::usb_pid,
    .bcdDevice = MATRIXOS_VERSION_ID_16,

    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,

    .bNumConfigurations = 0x01
  };

  return (uint8_t const*)&desc_device;
}

uint8_t const* default_configuration_descriptor_cb(uint8_t index) {
  (void)index;  // Ignore index like TinyUSB dynamic_configuration example
  return desc_default_configuration;
}

uint8_t const* default_hid_report_descriptor_cb(uint8_t instance) {
  (void) instance;
  return desc_hid_report;
}

static uint16_t _desc_str[32];

uint16_t const* default_string_descriptor_cb(uint8_t index, uint16_t langid) {
  (void)langid;

  uint8_t chr_count;

  string product_name = Device::product_name;

  if (MatrixOS::UserVar::device_id.Get())
  {
    product_name += " ";
    product_name += std::to_string(MatrixOS::UserVar::device_id.Get());
  }

  // array of pointer to string descriptors
  const char* string_desc_arr[] = {
      (const char[]){0x09, 0x04},              // 0: is supported language is English (0x0409)
      Device::manufacturer_name.c_str(),        // 1: Manufacturer
      product_name.c_str(),                    // 2: Product
      Device::GetSerial().c_str(),             // 3: Serials, should use chip ID
      (Device::product_name + " CDC").c_str(), // 4: CDC Interface
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