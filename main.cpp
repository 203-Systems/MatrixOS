#include "MatrixOS.h"
// #include "application/TestApp/TestApp.h"
#include "application/Performance/Performance.h"

int main()
{
    MatrixOS::SYS::Init();

    Performance Performance;
    Performance.main();

    return 0;             
}