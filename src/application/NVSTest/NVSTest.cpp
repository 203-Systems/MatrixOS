#include "NVSTest.h"
#include <algorithm>

void NVSTest::Setup()
{ 

}

bool inited = false;
void NVSTest::Loop()
{ 
  if(MatrixOS::USB::CDC::Connected() && !inited)
  {
    MatrixOS::Logging::LogInfo(name, "CDC Connected, print menu");
    PrintMenu();
    inited = true;
  }

  if (MatrixOS::USB::CDC::Available()) {
    // read the incoming byte:
    MatrixOS::Logging::LogInfo(name, "CDC got data");
    MatrixOS::USB::CDC::Println("---------------------------------------------------");
    char incomingByte = MatrixOS::USB::CDC::Read();
    tud_cdc_n_read_flush(0);
    // MatrixOS::USB::CDC::Println("Recived");
    switch(incomingByte)
    {
      case '0': //Print Menu
        // PrintMenu();
        break;
      case '1': //Clear
        MatrixOS::USB::CDC::Println("Format");
        Device::NVS::Clear();
        break;
      case '2':
      {
				MatrixOS::USB::CDC::Println("Read String from Hash Table");
				MatrixOS::USB::CDC::Println("Enter Key Name:");
				string str = WaitForString();
				std::vector<char> value = Device::NVS::Read(str);
				if (value.size() == 0)
				{
					MatrixOS::USB::CDC::Println("Key not found");
					break;
				}

				MatrixOS::USB::CDC::Print("Length: ");
        MatrixOS::USB::CDC::Println(std::to_string(value.size()).c_str());

				MatrixOS::USB::CDC::Print("Value: ");
				for (uint32_t i = 0; i < value.size(); i++)
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
        MatrixOS::USB::CDC::Println("Enter Key Name:");
        MatrixOS::USB::CDC::Flush();
				string name = WaitForString();
				MatrixOS::USB::CDC::Println("Enter Key Value String:");
        MatrixOS::USB::CDC::Flush();
				string value = WaitForString();
				const char* value_array = value.c_str();
				int length = value.length();
				//std::cout << length << std::endl;
        MatrixOS::USB::CDC::Print("Name: ");
        MatrixOS::USB::CDC::Println(name.c_str());
				MatrixOS::USB::CDC::Print("Value: ");
        MatrixOS::USB::CDC::Println(value_array);
				MatrixOS::USB::CDC::Print("Length: ");
        MatrixOS::USB::CDC::Println(std::to_string(length).c_str());

				Device::NVS::Write(name, (void*)value_array, length);
        MatrixOS::USB::CDC::Println("Data Wrote");
				break;
      }
      // case '4':
      // {
      //   // uint32_t value = 0xCBCBCBCB;
      //   // WriteToFlash(0x8070000, 4, (uint16_t*)&value);
      //   MatrixOS::USB::CDC::Println("Test Data wrote");
      //   break;
      // }
      default:
        MatrixOS::USB::CDC::Println("Undefined Function");
        // MatrixOS::USB::CDC::Flush();
        break;
    }
    PrintMenu();
  }
}

void NVSTest::PrintMenu()
{
  MatrixOS::USB::CDC::Println("---------------------------------------------------");
  MatrixOS::USB::CDC::Println("Please enter actions that you want to perform:");
  MatrixOS::USB::CDC::Println("0: Print this menu again");
  MatrixOS::USB::CDC::Println("1: Clear NVS");
  MatrixOS::USB::CDC::Println("2: Read String");
  MatrixOS::USB::CDC::Println("3: Write String");
  MatrixOS::USB::CDC::Println("Enter the number of the acction you want to perform: ");
  MatrixOS::USB::CDC::Flush();
}

string NVSTest::WaitForString()
{
  while(!MatrixOS::USB::CDC::Available())
  {
    LoopTask();
  }
  string str = MatrixOS::USB::CDC::ReadString();
  StripString(str);
  return str;
} 

void NVSTest::StripString(string& str)
{
  std::remove(str.begin(), str.end(), '\n');
  // str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
  // str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
}