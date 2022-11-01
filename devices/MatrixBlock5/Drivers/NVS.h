#pragma once

#include "Device.h"
#include <map>

namespace Device::NVS
{

  struct HashKey {
    uint32_t hash;
    uint16_t length;
    uint16_t new_address = 0xFFFF;  // This is what limiting the max size. If more than 64 KB is needed, maybe change
                                    // this to uint32_t along with other changes

    HashKey() {
      this->hash = 0;
      this->length = 0;
    }

    HashKey(uint32_t hash, uint16_t length) {
      this->hash = hash;
      this->length = length;
    }

    bool Used() { return hash != 0xFFFFFFFF; }
    bool Latest() { return new_address == 0xFFFF; }
  };

  const uint32_t direction_indicator = 0x28dc67e2;

  void Init();
  void UpdateBytesUsed();

  uint16_t FindKey(uint32_t hash);

  HashKey* GetKey(uint16_t virtual_address);
  HashKey* GetKey(uint8_t page, uint16_t local_address);
  uint16_t GetVirtualAddress(HashKey* hashKey);
  uint32_t GetPage(uint8_t index);
  uint16_t GetFreeSpace(uint8_t page);
  int8_t CheckSpace(uint16_t length);

  void CleanUpTable(uint32_t hash_to_ignore = 0);

  bool WriteKey(uint32_t hash, void* pointer, uint16_t length);

  void WriteToFlash(uint32_t pointer, uint16_t length, uint16_t* address);
  void EreasePage(uint32_t address, uint32_t pages = 1);

  // std::vector<char> Read(std::string name);
  // bool Write(std::string name, void* pointer, uint16_t length);
  // void Format();
}