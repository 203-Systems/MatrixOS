#pragma once

#include "MatrixOS.h"
#include "UsbInterface.h"

namespace MatrixOS::USB
{
  inline uint8_t outEndpointCount = 0;
  inline uint8_t inEndpointCount = 0;
  inline uint8_t interfaceCount = 0;

  inline deque<string> string_desc_arr;

  inline list<vector<uint8_t>> desc_configurations_temp;
  inline vector<uint8_t> desc_configurations;

  inline list <UsbInterface*> interfaces;

  void Init();

  enum class EndpointType:uint8_t { Out = 0, In = 1 };

  uint8_t RequestInterface();
  uint8_t RequestEndpoint(EndpointType input);

  uint8_t AddString(const string& str);
  void AddInterfaceDescriptor(const vector<uint8_t>& desc);

  bool AddInterface(UsbInterface* usbInterface);  // Add USB interface to the stack
}