#include "MatrixOS.h"

namespace MatrixOS::NVS
{
  size_t GetSize(uint32_t hash) {
    return Device::NVS::Size(hash);
  }
  
  vector<char> GetVariable(uint32_t hash) {
    return Device::NVS::Read(hash);
  }

  int8_t GetVariable(uint32_t hash, void* pointer, uint16_t length) {
    vector<char> data = Device::NVS::Read(hash);
    if (data.size() == 0)  // Havn't been saved
    {
      SetVariable(hash, pointer, length);
      return 1;
    }
    else if (data.size() != length)  // Size mismatched
    { return 2; }
    else if (data.size() == length)  // Size matched
    {
      memcpy(pointer, data.data(), length);
      return 0;
    }
    return -1;
  }

  bool SetVariable(uint32_t hash, void* pointer, uint16_t length) {
    // MLOGV("NVS", "Variable wrote : 0x%08x : 0x%08x: %d", hash, *(uint32_t*)pointer, length);
    // //Sometimes this will cause kernal panic
    return Device::NVS::Write(hash, pointer, length);
  }

  bool DeleteVariable(uint32_t hash) {
    return Device::NVS::Delete(hash);
  }
}