#include "Device.h"

namespace Device::NVS
{
  void Init() {
    // Stub: NVS not implemented for STM32F103
  }

  void Clear() {
    // Stub: NVS not implemented for STM32F103
  }

  size_t Size(uint32_t hash) {
    // Stub: NVS not implemented for STM32F103
    return 0;
  }

  vector<char> Read(uint32_t hash) {
    // Stub: NVS not implemented for STM32F103
    return vector<char>();
  }

  bool Write(uint32_t hash, void* pointer, uint16_t length) {
    // Stub: NVS not implemented for STM32F103
    return false;
  }

  bool Delete(uint32_t hash) {
    // Stub: NVS not implemented for STM32F103
    return false;
  }
}
