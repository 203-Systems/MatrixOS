#include "tusb.h"

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "MatrixOS.h"

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+

tusb_desc_device_t desc_device;
// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void) {
  desc_device = {.bLength = sizeof(tusb_desc_device_t),
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
                 .bcdDevice = (uint16_t)((MATRIXOS_MAJOR_VER << 8) + (MATRIXOS_MINOR_VER << 4) + (MATRIXOS_PATCH_VER)),  // If this is too limiting, we can then use the first 4 bits

                 .iManufacturer = 0x01,
                 .iProduct = 0x02,
                 .iSerialNumber = 0x03,

                 .bNumConfigurations = 0x01};

  return (uint8_t const*)&desc_device;
}

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
      /* Report ID for Inquiry */\
      0x85, 255, \
      /* Feature Report (64 bytes) */ \
      HID_USAGE   ( 0x80                                    ),\
      HID_LOGICAL_MIN ( 0                                       ),\
      HID_LOGICAL_MAX ( 255                                     ),\
      HID_REPORT_COUNT( 64                                      ),\
      HID_FEATURE       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE),\
      /* Input Report (64 bytes) */ \
      HID_USAGE   ( 0x10                                    ),\
      HID_LOGICAL_MIN ( 0                                       ),\
      HID_LOGICAL_MAX ( 255                                     ),\
      HID_REPORT_SIZE ( 8                                       ),\
      HID_REPORT_COUNT( 16                                      ),\
      HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ),\
      /* Output Report (64 bytes) */ \
      HID_USAGE   ( 0x10                                    ),\
      HID_LOGICAL_MIN ( 0                                       ),\
      HID_LOGICAL_MAX ( 255                                     ),\
      HID_REPORT_SIZE ( 8                                       ),\
      HID_REPORT_COUNT( 16                                      ),\
      HID_OUTPUT      ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ),\
    HID_COLLECTION_END
  

uint8_t const desc_hid_report[] = {TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)), 
                                   TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE)),
                                   TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL)), 
                                   HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(REPORT_ID_GAMEPAD)),
                                   HID_REPORT_DESC_VENDOR()
                                  };

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance)
{
  (void) instance;
  return desc_hid_report;
}

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

enum 
{ ITF_NUM_MIDI = 0,
  ITF_NUM_MIDI_STREAMING, 
  ITF_NUM_CDC, 
  ITF_NUM_CDC_DATA, 
  ITF_NUM_HID,
  ITF_NUM_TOTAL 
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_DUO_MIDI_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

#define EPNUM_MIDI 0x01
#define EPNUM_CDC_NOTIF 0x82
#define EPNUM_CDC_OUT   0x02
#define EPNUM_CDC_IN    0x83
#define EPNUM_HID_OUT   0x04
#define EPNUM_HID_IN    0x84

uint8_t const desc_fs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_DUO_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 0, EPNUM_MIDI, 0x80 | EPNUM_MIDI, 64),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),

    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID_OUT, EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 5)
  };

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;  // for multiple configurations

  return desc_fs_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// char manufacturer_name[Device::manufacturer_name.length()] = Device::manufacturer_name.c_str();
// char product_name[Device::product_name.length()] = Device::product_name.c_str();
// char device_serial[Device::GetSerial().length()] = Device::GetSerial().c_str();
// char usb_cdc_name[Device::product_name.length() + 4] = (Device::product_name + " CDC").c_str();

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;

  uint8_t chr_count;

  string product_name = Device::product_name;

  if (MatrixOS::UserVar::device_id.Get())
  {
    product_name += " ";
    product_name += std::to_string(MatrixOS::UserVar::device_id.Get());
  }

  string serial_number = Device::GetSerial();

  // array of pointer to string descriptors
  const char* string_desc_arr[] = {
      (const char[]){0x09, 0x04},              // 0: is supported language is English (0x0409)
      Device::manufacturer_name.c_str(),        // 1: Manufacturer
      product_name.c_str(),                    // 2: Product
      serial_number.c_str(),                   // 3: Serials, should use chip ID
      (Device::product_name + " CDC").c_str()  // 4: CDC Interface
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