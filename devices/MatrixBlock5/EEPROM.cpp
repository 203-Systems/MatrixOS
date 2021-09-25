#include "EEPROM.h"

namespace Device::EEPROM
{
	uint16_t byteUsed[nums_of_page];
	bool reversed = false;

	void Init()
	{	
		//Determine Data Direction
		reversed = false;
		uint32_t left = *(uint32_t*)GetPage(0);
		uint32_t right = *(uint32_t*)GetPage(nums_of_page - 1);
		if (left == direction_indicator && right != direction_indicator)
		{
			// std::cout << "EEPROM is currently in forward direction" << std::endl;
			reversed = false;
		}
		else if (left != direction_indicator && right == direction_indicator)
		{
			// std::cout << "EEPROM is currently in reverse direction" << std::endl;
			reversed = true;
		}
		else
		{
			// std::cout << "EEPROM is current uninitalized or corrupted" << std::endl;
			Format();
		}
		return;


		//Get Byte Used in Each Page
		for (uint8_t page = 0; page < nums_of_page; page++)
		{
			uint32_t page_address = GetPage(page);
			uint16_t local_address = 0;
			if (page == 0) local_address = 4;
			while (local_address < page_size)
			{
				HashKey* hashKey = (HashKey*)(page_address + local_address);
				if (hashKey->Used() == false)
				{
					byteUsed[page] = local_address;
					break;
				}
				local_address += sizeof(HashKey) + hashKey->length;
			}
		}
	}

	std::vector<char> Read(std::string name)
	{
		//printf("Reading Key\n");
		uint32_t hash = fnv1a_hash(name.c_str());
		uint16_t virtual_address = FindKey(hash);
		if (virtual_address == 0xFFFF) return std::vector<char>(0);
		HashKey* hashKey = GetKey(virtual_address);
		//printf("Key Found, length: %d\n", hashKey->length);
		std::vector<char> value(hashKey->length);
		memcpy(value.data(), (char*)hashKey + sizeof(HashKey), hashKey->length);
		return value;
	}

	bool Write(std::string name, void* pointer, uint16_t length)
	{
		//printf("EEPROM Write event - Key: %s, value %s, pointer %p, length %u\n", name.c_str(), (char*)pointer, pointer, length);
		// Iterate through the Table section to find matching key 
		uint32_t hash = fnv1a_hash(name.c_str());
		return WriteKey(hash, pointer, length);
	}

	bool WriteKey(uint32_t hash, void* pointer, uint16_t length)
	{
		uint16_t oldAddress = FindKey(hash);
		//Find Which Page to Write
		int8_t page = CheckSpace(length);
		if (page == -1)
		{
			CleanUpTable();
			page = CheckSpace(length);
			if (page == -1) return false;
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
		byteUsed[page] += (length + (length % 2)) + sizeof(HashKey); //16bit align

		// Update Old Key
		if (oldAddress != 0xFFFF)
		{
			HashKey* oldKey = GetKey(oldAddress);
			uint16_t virtual_address = page * page_size + local_address;
			WriteToFlash((void*)(&oldKey->new_address), 2, &virtual_address);
		}
		return true;
	}
	int8_t CheckSpace(uint16_t length)
	{
		for (uint8_t page = 0; page < nums_of_page; page++)
		{	
			//printf("Page %u free space %u\n", page, GetFreeSpace(page));
			if (GetFreeSpace(page) < length + sizeof(HashKey)) continue;
			return page;
		}
		return -1;
	}

	uint16_t FindKey(uint32_t hash)
	{
		for (int8_t page = nums_of_page - 1; page >= 0; page--) {
			char* page_address = (char*)GetPage(page);
			uint16_t local_address = page == 0 ? 4 : 0;
			while (local_address < page_size)
			{
				HashKey* hashKey = (HashKey*)(page_address + local_address);
				if (hashKey->Used() == false) break; //Avoid Hole
				if (hashKey->hash == hash)
				{
					while (hashKey->Latest() == false)
					{
						hashKey = GetKey(hashKey->new_address);
					}
					//printf("Key Index %X\n", GetVirtualAddress(hashKey));
					return GetVirtualAddress(hashKey);
				}
				local_address += sizeof(HashKey) + hashKey->length + hashKey->length % 2;//16bit aligned
			}
		}
		//printf("Key Not found\n");
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
		uint32_t eeprom_address_offset = (uint32_t)((char*)hashKey - eeprom_address); //Address across pages
		uint8_t page = eeprom_address_offset / page_size;
		if (reversed) page = (nums_of_page - 1) - page;
		uint16_t local_address = eeprom_address_offset % page_size;
		return page * page_size + local_address;
	}


	uint32_t GetPage(uint8_t index)
	{
		uint8_t actual_page_index = reversed ? (nums_of_page - 1) - index : index;
		return eeprom_address + actual_page_index * page_size;
	}

	uint16_t GetFreeSpace(uint8_t page)
	{
		return page_size - byteUsed[page] - (((page + 1) == nums_of_page) ? 4 : 0); //Leaves 4 byte at the end page open. This space is resvered for Data Direction Indicator.
	}

	void CleanUpTable(uint32_t hash_to_ignore)
	{
		// printf("Clean Up Table\n");
		reversed = !reversed;
		for (uint8_t page = 0; page < nums_of_page; page++)  //Read from back to front (On previous table's perspective)
		{
			std::map<uint32_t, std::vector<char>> pageData;
			uint32_t page_address = GetPage(page);

			uint16_t local_address = page == (nums_of_page - 1) ? 4 : 0; //Previous header has indicator, offset needed.
			while (local_address < page_size)
			{
				HashKey* hashKey = (HashKey*)(page_address + local_address);
				if (hashKey->Used() == false) break; //End of page reached
				local_address += sizeof(HashKey) + hashKey->length + hashKey->length % 2; //16bit aligned
				if (hashKey->Latest() == false) continue; //Ignore keys that are out dated
				if (hashKey->hash == hash_to_ignore) continue; //Ignore hash that is about to be write
				std::vector<char> value(hashKey->length);
				// printf("Key value found - Hash:0x%X, Length:%u, Value:", hashKey->hash, hashKey->length);
				for (int i = 0; i < hashKey->length; i++)
				{
					printf("%c", ((char*)(hashKey) + sizeof(HashKey))[i]);
				}
				// std::cout << std::endl;
				memcpy(value.data(), (char*)hashKey + sizeof(HashKey), hashKey->length);
	/*			std::cout << "Vector data: " << std::endl;
				for (int i = 0; i < hashKey->length; i++)
				{
					printf("%c", value.at(i));
				}*/
				// std::cout << std::endl;
				pageData[hashKey->hash] = value;
			}

			//Clean Page
			EreasePage(page_address);

			byteUsed[page] = 0;

			if (page == 0)
			{
				byteUsed[page] = 4;
				WriteToFlash((void*)page_address, 4, (uint16_t*)&direction_indicator);
			}

			for (const auto &data : pageData) {
				uint32_t hash = data.first;
				std::vector<char> value = data.second;
				WriteKey(hash, value.data(), value.size());
			}
		}
	}

	void Format()
	{
		// std::cout << "EEPROM Formating..." << std::endl;
		memset((void*)eeprom_address, 0xFF, page_size * nums_of_page);
		WriteToFlash((void*)eeprom_address, 4, (uint16_t*)&direction_indicator);

		reversed = false;
		byteUsed[0] = 4;
		for (uint8_t page = 1; page < nums_of_page; page++)
		{
			byteUsed[page] = 0;
		}
		// std::cout << "EEPROM Formated" << std::endl;
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

    void EreasePage(uint32_t address, uint32_t pages)
	{
        uint32_t error;

        HAL_FLASH_Unlock();
        FLASH_EraseInitTypeDef flashErase;
        flashErase.NbPages = pages;
        flashErase.PageAddress = address;
        flashErase.TypeErase = FLASH_TYPEERASE_PAGES;
        HAL_FLASHEx_Erase(&flashErase, &error);
        WriteToFlash((void*)eeprom_address, 4, (uint16_t*)&direction_indicator);
        HAL_FLASH_Lock();
	}
}