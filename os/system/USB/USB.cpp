#include "USB.h"
#include "MIDI.h"
#include "MatrixOS.h"

namespace MatrixOS::USB
{

  void usb_device_task(void* param) {
    (void)param;
    // RTOS forever loop
    while (1)
    {
      // tinyusb device task
      tud_task();
    }
  }

  // Create a task for tinyusb device stack
#define USBD_STACK_SIZE (3 * configMINIMAL_STACK_SIZE)
  StackType_t usb_device_stack[USBD_STACK_SIZE];
  StaticTask_t usb_device_taskdef;
  USB::MIDI midi = USB::MIDI(8, "Matrix Midi", 64);
  void Init() {
    tusb_init();
    tud_disconnect();
    Begin();
    (void)xTaskCreateStatic(usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, usb_device_stack, &usb_device_taskdef);
  }

  bool Inited() {
    return tusb_inited();
  }

  bool Disconnect() {
    return tud_disconnect();
  }

  bool Connect() {
    return tud_connect();
  }

  bool Connected() {
    return tud_ready();
  }

  bool AddInterface(UsbInterface* usbInterface) {
    USB::interfaces.push_back(usbInterface);
    return true;
  }

  bool Begin(void) {

    // Prepare string_desc_arr
    string product_name = Device::product_name;
    if (MatrixOS::UserVar::device_id.Get())
    {
      product_name += " ";
      product_name += std::to_string(MatrixOS::UserVar::device_id.Get());
    }

    // Add strings in reverse order!!!
    string_desc_arr.push_front(Device::GetSerial());
    string_desc_arr.push_front(product_name);
    string_desc_arr.push_front(Device::manufacturer_name);
    string_desc_arr.push_front((const char[]){0x09, 0x04});

    // Print string descriptors
    ESP_LOGI("USB", "String descriptors: ");
    // for (uint8_t i = 0; i < string_desc_arr.size(); i++)
    // {
    //   ESP_LOGI("USB", "%d: %s", i, string_desc_arr[i].c_str());
    // }

    // Prepare desc_configurations
    // Calculate total length
    uint16_t config_total_length = TUD_CONFIG_DESC_LEN;

    // Calculate total length
    for (vector<uint8_t> desc_configuration : desc_configurations_temp)
    {
      config_total_length += desc_configuration.size();
    }

    ESP_LOGI("USB", "Total length: %d", config_total_length);
    ESP_LOGI("USB", "Interface count: %d", interfaceCount);

    // Allocate descriptor
    desc_configurations.reserve(config_total_length);

    // Add config descriptor
    // Config number, interface count, string index, total length, attribute, power in mA
    uint8_t config_desc[TUD_CONFIG_DESC_LEN] = {TUD_CONFIG_DESCRIPTOR(1, interfaceCount, 0, config_total_length, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500)};
    desc_configurations.insert(desc_configurations.end(), config_desc, config_desc + TUD_CONFIG_DESC_LEN);

    // Add interface descriptors
    for (vector<uint8_t> desc_configuration : desc_configurations_temp)
    {
      desc_configurations.insert(desc_configurations.end(), desc_configuration.begin(), desc_configuration.end());
    }

    // printf("Configurations: \n");
    // for (uint8_t i = 0; i < desc_configurations.size(); i++)
    // {
    //   printf("%02X ", desc_configurations[i]);
    // }
    // printf("\n");

    // Free temp
    desc_configurations_temp.clear();

    // Connect
    tud_connect();

    return true;
  }

  void End(void) {
    // Disconnect
    tud_disconnect();

    // Reset endpoints
    desc_configurations_temp.clear();
    desc_configurations.clear();

    // Reset string descriptors
    string_desc_arr.clear();
  }

  uint8_t RequestInterface() {  // TODO, Add max interface limit
    return interfaceCount++;    // Starts from 0x00
  }

  uint8_t RequestEndpoint(EndpointType input) {  // TODO, Add max endpoint limit
    if (input == EndpointType::In)
    {
      return 0x80 + ++inEndpointCount;  // Starts from 0x81
    }
    else
    {
      return ++outEndpointCount;  // Starts from 0x01
    }
  }

  uint8_t AddString(const string& str) {
    ESP_LOGI("USB", "Adding string: %s", str.c_str());
    if (str.empty())
      return 0;  // Uses default product name string

    // Check if exists
    {
      int index = 0;
      for (auto it = string_desc_arr.begin(); it != string_desc_arr.end(); ++it, ++index) {
          if (*it == str) {
              return index + 4; // Found the string, add offset for the first 4 strings
          }
      }
    }

    string_desc_arr.push_back(str);
    return static_cast<uint8_t>(string_desc_arr.size() - 1 + 4);  // Add offset for the first 4 strings
  }

  void AddInterfaceDescriptor(const vector<uint8_t>& desc)
  {
    // ESP_LOGI("USB", "Adding interface descriptor: ");
    // for (uint8_t i = 0; i < desc.size(); i++)
    // {
    //   printf("%02X ", desc[i]);
    // }
    // printf("\n");
    desc_configurations_temp.push_back(desc);
  }
}
//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {}

// Invoked when device is unmounted
void tud_umount_cb(void) {}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {}

//--------------------------------------------------------------------+
// USB DESCRIPTOR CALLBACKS
//--------------------------------------------------------------------+
tusb_desc_device_t desc_device;
// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void) {
  ESP_LOGI("USB", "GET DEVICE DESCRIPTOR request");
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

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;  // for multiple configurations
  ESP_LOGI("USB", "GET CONFIGURATION DESCRIPTOR request - Index: %d", index);

  return MatrixOS::USB::desc_configurations.data();
}

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  // Clear string descriptor
  memset(_desc_str, 0, sizeof(_desc_str));

  uint8_t chr_count;

  if ( index == 0)
  {
    memcpy(&_desc_str[1], MatrixOS::USB::string_desc_arr[0].c_str(), 2);
    chr_count = 1;
  }else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if ( !(index < MatrixOS::USB::string_desc_arr.size())) return NULL;

    const char* str = MatrixOS::USB::string_desc_arr[index].c_str();

    // Cap at max char
    chr_count = (uint8_t) strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    // Convert ASCII string into UTF-16
    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8 ) | (2*chr_count + 2));

  return _desc_str;
}