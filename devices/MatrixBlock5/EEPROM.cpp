#include "EEPROM.h"

namespace Device::EEPROM
{
    uint16_t byteUsed[nums_of_page];
    bool reversed = false;

    const uint32_t direction_indicator = 0x28dc67e2;

    void Init()
    {
        //Determine Data Direction
        uint32_t left = *(uint32_t*)GetPage(0);
        uint32_t right = *(uint32_t*)GetPage(nums_of_page-1);
        if(left == direction_indicator && right != direction_indicator)
        {
            reversed = false;
        }
        else if(left != direction_indicator && right == direction_indicator)
        {
            reversed = true;
        }
        else
        {
            Format();
        }


        //Get Byte Used in Each Page
        for(uint8_t page = 0; page < nums_of_page; page++)
        {
            uint32_t page_address = GetPage(page);
            uint16_t local_address = 0;
            if(page == 0) local_address = 4;
            while(local_address < page_size)
            {
                HashKey* hashKey = (HashKey*)(page_address + local_address);
                if(hashKey->Used() == false)
                {
                    byteUsed[page] = local_address;
                    break;
                }
                local_address += sizeof(HashKey) + hashKey->length;
            }
        }
    }

    void* Read(std::string name)
    {
        uint32_t hash = fnv1a_hash(name.c_str());
        return (void*)((char*)GetKey(FindKey(hash)) + sizeof(HashKey));
    }

    bool Write(std::string name, void* pointer, uint16_t length)
    {   
        // Iterate through the Table section to find matching key 
        uint32_t hash = fnv1a_hash(name.c_str());
        HashKey* oldKey = GetKey(hash);
        //Find Which Page to Write
        int8_t page = CheckSpace(length);
        if(page == -1)
        {
            CleanUpTable();
            page = CheckSpace(length);
            if(page == -1) return false;
        }

        // Write Key
        // Create New Key
        HashKey newKey = HashKey(hash, length);
        // Write HashKey to Flash
        uint16_t local_address = byteUsed[page];
        uint32_t write_address = GetPage(page) + local_address;
        WriteToFlash((void*)write_address, sizeof(HashKey), (uint16_t*)&newKey);
        // Write Actual Data
        write_address += sizeof(HashKey);
        WriteToFlash((void*)write_address, length, (uint16_t*)pointer);
        // Update Page Usage
        byteUsed[page] += (length + (length % 2) * 2) + sizeof(HashKey);

        // Update Old Key
        uint16_t virtual_address = page * page_size + local_address;
        WriteToFlash((void*)(&oldKey->new_address), 2, &virtual_address);
        return true;
    }

    int8_t CheckSpace(uint16_t length)
    {
        for(uint8_t page = 0; page < nums_of_page; page++)
        {
            if(GetFreeSpace(page) < length + sizeof(HashKey)) continue;
            return page;
        }
        return -1;
    }   

    uint16_t FindKey(uint32_t hash)
    {
        for(uint8_t page = 0; page < nums_of_page; page ++){
            uint16_t local_address = 0;
            while(true)
            {
                HashKey* hashKey = (HashKey*)((char*)GetPage(page) + local_address);
                if(hashKey->Used() == false) break; //Avoid Hole
                if(hashKey->hash == hash)
                {   
                    while(hashKey->Latest() == false)
                    {
                        hashKey = GetKey(hashKey->new_address);
                    }
                    return GetVirtualAddress(hashKey);
                }
                local_address += sizeof(HashKey) + hashKey->length;
            }
        }
        return 0xFFFF; //oops we ran out of keys and still can't find it. Use 0xFFFF because it's impossible as a key address
    }


    HashKey* GetKey(uint16_t virtual_address)
    {
        return GetKey(virtual_address / page_size, virtual_address % page_size);
    }

    HashKey* GetKey(uint8_t page, uint16_t local_address)
    {
        return (HashKey*)(GetPage(page) + local_address);
    }

    uint16_t GetVirtualAddress(HashKey* hashKey)
    {
        uint32_t eeprom_address = (uint32_t)((char*)hashKey - eeprom_address); //Address across pages
        uint8_t page = eeprom_address / page_size;
        if(reversed) page = (nums_of_page - 1) - page;
        uint16_t local_address = eeprom_address % page_size;
        return page * page_size + local_address;
    }


   uint32_t GetPage(uint8_t index)
    {
        uint8_t actual_page_index = reversed ? (nums_of_page - 1) - index : index;
        return eeprom_address + actual_page_index * page_size;
    }

    uint16_t GetFreeSpace(uint8_t page)
    {
        return page_size - byteUsed[page] - ((page + 1) == nums_of_page) ? 4 : 0; //Leaves 4 byte at the end page open. This space is resvered for Data Direction Indicator.
    }

    void CleanUpTable()
    {
        
    }

    void Format()
    {
        uint32_t error;

        HAL_FLASH_Unlock();
        FLASH_EraseInitTypeDef flashErase;
        flashErase.NbPages = nums_of_page;
        flashErase.PageAddress = eeprom_address;
        flashErase.TypeErase = FLASH_TYPEERASE_PAGES;
        HAL_FLASHEx_Erase(&flashErase, &error);
        WriteToFlash((void*)eeprom_address, 4, (uint16_t*)&direction_indicator);
        HAL_FLASH_Lock();

        reversed = false;
        byteUsed[0] = 4;
        for(uint8_t page = 1; page < nums_of_page; page++)
        {
            byteUsed[page] = 0;
        }

    }

    void WriteToFlash(void* pointer, uint16_t length, uint16_t* address)
    {
        uint16_t length_16bit = length / 2;
        uint16_t* pointer_16bit = (uint16_t*)pointer;

        HAL_FLASH_Unlock();
        for(uint16_t i = 0; i < length_16bit; i ++)
        {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (uint32_t)(address + i), pointer_16bit[i]);
        }
        if(length % 2) //If the data length is odd number
        {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (uint32_t)(address + length_16bit), pointer_16bit[length_16bit] & 0xF0);
        }
        HAL_FLASH_Lock();
    }
}