#include "MatrixOS.h"

namespace MatrixOS::SYS
{
    void Begin(void);
}

int main()
{
    MatrixOS::SYS::Begin();
    vTaskDelete(NULL);

    return 0;             
}