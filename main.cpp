#include "MatrixOS.h"
#include "application/TestApp/TestApp.h"
#include "application/Performance/Performance.h"

int main()
{
    MatrixOS::SYS::Init();

    Performance performance;
    performance.Start();
    // TestApp testApp;
    // testApp.Start();

    return 0;             
}