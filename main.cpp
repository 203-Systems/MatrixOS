#include "MatrixOS.h"
#include "application/Applications.h"

int main()
{
    MatrixOS::SYS::Init();

    EEPROMTest eepromTest;
    eepromTest.Start();

    return 0;             
}

extern "C"{ void * __dso_handle = 0 ;}  