#include "system/MatrixOS.h"
#include "application/TestApp/TestApp.h"

int main()
{
    MatrixOS::SYS::Init();

    TestApp();

    return 0;             
}
