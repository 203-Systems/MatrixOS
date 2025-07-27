#include "MatrixOS.h"

int main()
{
    MatrixOS::SYS::Begin();
    vTaskDelete(NULL);

    return 0;             
}