#include "EEPROMTest.h"
#include <string> 
#include <algorithm>
#include "EEPROM.h"

void EEPROMTest::Setup()
{ 

}

bool inited = false;
void EEPROMTest::Loop()
{ 
  if(MatrixOS::USB::CDC::Connected() && !inited)
  {
    PrintMenu();
    inited = true;
  }

  if (MatrixOS::USB::CDC::Available()) {
    // read the incoming byte:
    MatrixOS::USB::CDC::Println("---------------------------------------------------");
    char incomingByte = MatrixOS::USB::CDC::Read();
    tud_cdc_n_read_flush(0);
    // MatrixOS::USB::CDC::Println("Recived");
    switch(incomingByte)
    {
      case '0':
        PrintMenu();
        break;
      case '1':
        // MatrixOS::USB::CDC::Println("Format");
        // Device::EEPROM::Init();
        // Device::EEPROM::UpdateBytesUsed();
        break;
      case '2':
      {
				MatrixOS::USB::CDC::Println("Read String from Hash Table");
				MatrixOS::USB::CDC::Println("Enter Key Name:");
				std::string str = WaitForString();
				std::vector<char> value = Device::EEPROM::Read(str);
				if (value.size() == 0)
				{
					MatrixOS::USB::CDC::Println("Key not found");
					break;
				}

				MatrixOS::USB::CDC::Printf("Length: ");
        MatrixOS::USB::CDC::Println(std::to_string(value.size()).c_str());

				MatrixOS::USB::CDC::Print("Value: ");
				for (int i = 0; i < value.size(); i++)
				{
          // MatrixOS::USB::CDC::Print(&value[0]);
          tud_cdc_n_write_char(0, value[i]);
				}
        MatrixOS::USB::CDC::Println("");
				break;
      }
      case '3':
      {
        MatrixOS::USB::CDC::Println("Write String to Hash Table");
				//MatrixOS::USB::CDC::Println("Enter Length: ";
				//int length;
				//std::cin >> length;
				MatrixOS::USB::CDC::Println("Enter Key Name:");
        MatrixOS::USB::CDC::Flush();
				std::string name = WaitForString();
				MatrixOS::USB::CDC::Println("Enter Key Value String:");
        MatrixOS::USB::CDC::Flush();
				std::string value = WaitForString();
				const char* value_array = value.c_str();
				int length = value.length();
				//std::cout << length << std::endl;
        MatrixOS::USB::CDC::Print("Name: ");
        MatrixOS::USB::CDC::Println(name.c_str());
				MatrixOS::USB::CDC::Print("Value: ");
        MatrixOS::USB::CDC::Println(value_array);
				MatrixOS::USB::CDC::Print("Length: ");
        MatrixOS::USB::CDC::Println(std::to_string(length).c_str());

				Device::EEPROM::Write(name, (void*)value_array, length);
        MatrixOS::USB::CDC::Println("Data Wrote");
				break;
      }
      case '4':
        MatrixOS::USB::CDC::Println("Action 4");
        // MatrixOS::USB::CDC::Flush();
        break;
      case '5':
      {
        MatrixOS::USB::CDC::Println("Enter Start Address");
        uint32_t start_address = WaitForHex();
        MatrixOS::USB::CDC::Print("Address: ");
        MatrixOS::USB::CDC::Println(std::to_string(start_address).c_str());
        MatrixOS::USB::CDC::Println("Enter Length");
        uint32_t length = WaitForHex();
        MatrixOS::USB::CDC::Println(std::to_string(length).c_str());
        MatrixOS::USB::CDC::Println("Data:");
        for(uint32_t i = 0; i < length; i++)
        {
          if(i > 0 && i % 16 == 0)
            MatrixOS::USB::CDC::Println("");
          const volatile uint8_t value = *((uint8_t*)(start_address + i));
          char hex[3];
          bytes2hex((unsigned char*)&value, hex, 1);
          // const char* digits = "0123456789ABCDEF";
          // std::string str;
          // str.push_back(digits[value]);
          // str.push_back(' ');
          // MatrixOS::USB::CDC::Print(str.c_str());
          MatrixOS::USB::CDC::Print(hex);
          MatrixOS::USB::CDC::Print(" ");
        }
        MatrixOS::USB::CDC::Println(" ");
        // MatrixOS::USB::CDC::Flush();
        break;
      }
      case '6':
      {
        uint32_t value = 0xCBCBCBCB;
        WriteToFlash(0x8070000, 4, (uint16_t*)&value);
        MatrixOS::USB::CDC::Println("Test Data wrote");
      }
      case '7':
      {
        Device::EEPROM::Format();
        break;
      }
      default:
        MatrixOS::USB::CDC::Println("Undefined Function");
        // MatrixOS::USB::CDC::Flush();
        break;
    }
    PrintMenu();
  }
  // MatrixOS::USB::CDC::Println("Test");
  // MatrixOS::SYS::DelayMs(5);
}

void EEPROMTest::PrintMenu()
{
  MatrixOS::USB::CDC::Println("---------------------------------------------------");
  MatrixOS::USB::CDC::Println("Please enter actions that you want to perform:");
  MatrixOS::USB::CDC::Println("0: Print this menu");
  MatrixOS::USB::CDC::Println("1: Reinit");
  MatrixOS::USB::CDC::Println("2: Read String");
  MatrixOS::USB::CDC::Println("3: Write String");
  MatrixOS::USB::CDC::Println("4: Clean Up Table");
  MatrixOS::USB::CDC::Println("5: Dump Flash");
  MatrixOS::USB::CDC::Println("6: Write Test Data");
  MatrixOS::USB::CDC::Println("7: Format");
  // MatrixOS::USB::CDC::Println("Enter the number of the acction you want to perform: ");
  // MatrixOS::USB::CDC::Flush();
}

std::string EEPROMTest::WaitForString()
{
  while(!MatrixOS::USB::CDC::Available())
  {
    LoopTask();
  }
  std::string str = MatrixOS::USB::CDC::ReadString();
  StripString(str);
  return str;
} 

uint32_t EEPROMTest::WaitForHex()
{
    std::string s = WaitForString();
    char * p;
    uint32_t n = strtoul( s.c_str(), & p, 16 ); 
    if ( * p != 0 ) {  
        MatrixOS::USB::CDC::Println("Input Error");
    }
    return n;
}

void EEPROMTest::StripString(std::string& str)
{
  str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
  str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
}

void EEPROMTest::WriteToFlash(uint32_t flash_address, uint16_t length, uint16_t* data)
{
    MatrixOS::USB::CDC::Println("Write To Flash");
    uint16_t length_16bit = length / 2; //If odd then 1 less than actual size
    uint16_t* flash_address_16bit = (uint16_t*)flash_address;

    HAL_FLASH_Unlock();
    MatrixOS::USB::CDC::Println("Flash Unlocked");
    for(uint16_t i = 0; i < length_16bit; i ++)
    {
    MatrixOS::USB::CDC::Print("Write To Flash - Address: ");
    MatrixOS::USB::CDC::Print(std::to_string((uint32_t)(flash_address_16bit + i)).c_str());
    MatrixOS::USB::CDC::Print(" Value: ");
    MatrixOS::USB::CDC::Println(std::to_string((uint64_t)(data[i])).c_str());
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (uint32_t)(flash_address_16bit + i), (uint64_t)(data[i])) != HAL_OK)
			{
				MatrixOS::SYS::ErrorHandler("Flash Write ERROR");
			}
    }
    if(length % 2) //If the data length is odd number
    {
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, (uint32_t)(flash_address_16bit + length_16bit), (uint64_t)(data[length_16bit])) != HAL_OK)
			{
				MatrixOS::SYS::ErrorHandler("Flash Write ERROR");
			}	
    }
    HAL_FLASH_Lock();
}

char HexLookUp[] = "0123456789abcdef";    
void EEPROMTest::bytes2hex (unsigned char *src, char *out, uint8_t len)
{
    while(len--)
    {
        *out++ = HexLookUp[*src >> 4];
        *out++ = HexLookUp[*src & 0x0F];
        src++;
    }
    *out = 0;
}