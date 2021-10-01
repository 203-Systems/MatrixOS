#include "MatrixOS.h"
#include "application/Applications.h"

int main()
{
    MatrixOS::SYS::Init();

    // EEPROMTest eepromTest;
    // eepromTest.Start();
    Performance performance;
    performance.Setup();

    return 0;             
}