#pragma once

#include "Device.h"

namespace Device::EEPROM
{

    struct HashKey
    {
        uint32_t hash;
        uint16_t length;
        uint16_t new_address = 0xFFFF; // This is what limiting the max size. If more than 64 KB is needed, maybe change this to uint32_t along with other changes

        HashKey()
        {
            this->hash = 0;
            this->length = 0;
        }

        HashKey(uint32_t hash, uint16_t length)
        {
            this->hash = hash;
            this->length = length;
        }

        bool Used() {return hash != 0xFFFF;}
        bool Latest() {return new_address != 0xFFFF;}
    };

    void Init();

    uint16_t FindKey(uint32_t hash);

    HashKey* GetKey(uint16_t virtual_address);
    HashKey* GetKey(uint8_t page, uint16_t local_address);
    uint16_t GetVirtualAddress(HashKey* hashKey);
    uint32_t GetPage(uint8_t index);
    uint16_t GetFreeSpace(uint8_t page);
    int8_t CheckSpace(uint16_t length);

    uint16_t WriteKey(uint32_t hash, void* pointer, uint16_t length, uint8_t page);

    void WriteToFlash(void* pointer, uint16_t length, uint16_t* address);
    // void WriteData(HashKey newKey, uint8_t offset);

    void CleanUpTable(); 
    void Format();

    // void* Read(std::string name);
    // void Write(std::string name, void* pointer, uint16_t length);
}