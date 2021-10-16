#include "MatrixOS.h"
#include "application/Applications.h"

int main()
{
    MatrixOS::SYS::Init();

    while (true)
    {
        MatrixOS::SYS::SystemTask();
    }
    

    // EEPROMTest eepromTest;
    // eepromTest.Start();
    // Performance performance;
    // performance.Setup();

    return 0;             
}