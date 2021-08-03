#include "system/MatrixOS.h"
#include "application/TestApp/TestApp.h"

int main()
{
    MatrixOS::SYS::Init();

    TestApp TestApp;
    TestApp.main();

    return 0;             
}

