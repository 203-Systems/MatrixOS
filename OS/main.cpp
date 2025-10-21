#include "MatrixOS.h"

namespace MatrixOS::SYS
{
    void Begin(void);
}

int main()
{
    MatrixOS::SYS::Begin();

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        vTaskStartScheduler();
        while(1);
    } else {
        // ESP32: Scheduler already running, delete this task
        vTaskDelete(NULL);
    }

    return 0;
}