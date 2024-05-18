#pragma once

#include "MatrixOS.h"
#include "UsbInterface.h"
#include "usbd_core.h"

#define USB_BUS_ID 0

// TODO: Move to chip-specific config 
#define USB_MAX_EP 5 // Discounting EP0
#define USB_MAX_IN_EP 4
#define USB_MAX_OUT_EP 5

#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

namespace MatrixOS::USB
{
  // Make sure to add any new variables to USB::Init() !!!
  inline bool inited = false;
  inline bool connected = false;
  inline bool suspended = false;
  inline bool mounted = false;
  inline bool remoteWakeup = false;

  inline list<string> string_desc_arr;

  inline list<vector<uint8_t>> usb_interface_descs;
  inline vector<uint8_t> usb_endpoint_descs;

  inline list<usbd_interface> interfaces;

  #ifdef BIDIRECTIONAL_ENDPOINTS
  inline list<usbd_endpoint> in_endpoints;
  inline list<usbd_endpoint> out_endpoints;
  #else
  inline list<usbd_endpoint> endpoints;
  #endif

  inline vector<uint8_t> in_endpoints_addr;
  inline vector<uint8_t> out_endpoints_addr;

  inline vector<uint8_t> out_endpoints_transfer_addr;
  inline vector<uint8_t*> out_endpoints_transfer_ptr;
  inline vector<uint8_t> out_endpoints_transfer_size;

  void Init();

  enum class EndpointType:uint8_t { Out = 0, In = 1 };

  usbd_interface* AddInterface();
  usbd_endpoint* AddEndpoint(EndpointType type, usbd_endpoint_callback cb, uint16_t ep_size = CONFIG_USB_DEFAULT_EP_SIZE);
  void AddOutEndpointInitTransfer(uint8_t out_endpoint_addr, uint8_t* out_endpoint_transfer_ptr, uint8_t out_endpoint_transfer_size);
  uint8_t AddString(const string& str);

  void AddInterfaceDescriptor(const vector<uint8_t>& desc);

  // bool AddInterface(UsbInterface* usbInterface);  // Add USB interface to the stack

  uint8_t* CompileDescriptor();

  // Status
  bool Mounted(void);
  bool Suspended(void);
  bool Ready(void);
  bool RemoteWakeup(void);

  void DeviceEventHandler(uint8_t busid, uint8_t event);
}