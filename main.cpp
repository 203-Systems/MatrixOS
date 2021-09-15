#include "MatrixOS.h"
#include "application/Performance/Performance.h"

int main()
{
    MatrixOS::SYS::Init();

    Performance performance;
    performance.Start();

    return 0;             
}

extern "C"{ void * __dso_handle = 0 ;}  