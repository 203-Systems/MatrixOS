#include "EEPROMTest.h"
#include <string> 

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
    char incomingByte = MatrixOS::USB::CDC::Read();
    tud_cdc_n_read_flush(0);
    // MatrixOS::USB::CDC::Println("Recived");
    switch(incomingByte)
    {
      case '0':
        PrintMenu();
        break;
      case '1':
        MatrixOS::USB::CDC::Println("Action 1");
        // MatrixOS::USB::CDC::Flush();
        break;
      case '2':
        MatrixOS::USB::CDC::Println("Action 2");
        // MatrixOS::USB::CDC::Flush();
        break;
      case '3':
        MatrixOS::USB::CDC::Println("Action 3");
        // MatrixOS::USB::CDC::Flush();
        break;
      case '4':
        MatrixOS::USB::CDC::Println("Action 4");
        // MatrixOS::USB::CDC::Flush();
        break;
      default:
        MatrixOS::USB::CDC::Println("Undefined Function");
        // MatrixOS::USB::CDC::Flush();
    }
  }
  // MatrixOS::USB::CDC::Println("Test");
  // MatrixOS::SYS::DelayMs(5);
}

void EEPROMTest::PrintMenu()
{
  MatrixOS::USB::CDC::Println("Please enter actions that you want to perform:");
  MatrixOS::USB::CDC::Println("0: Print this menu");
  MatrixOS::USB::CDC::Println("1: Reinit");
  MatrixOS::USB::CDC::Println("2: Read String");
  MatrixOS::USB::CDC::Println("3: Write String");
  MatrixOS::USB::CDC::Println("4: Clean Up Table");
  // MatrixOS::USB::CDC::Println("Enter the number of the acction you want to perform: ");
  // MatrixOS::USB::CDC::Flush();
}