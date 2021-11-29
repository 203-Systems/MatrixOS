#include "MatrixOS.h"
#include "application/Applications.h"


#define APPLICATION_STACK_SIZE     (configMINIMAL_STACK_SIZE * 16)
StackType_t  application_stack[APPLICATION_STACK_SIZE];
StaticTask_t application_taskdef;
void Application(void* param)
{
    // NVSTest nvsTest;
    // nvsTest.Start();
    Performance performance;
    performance.Start();

    // TestApp TestApp;
    // TestApp.Start();

    // while(true)
    // {
    //     // MatrixOS::Logging::LogDebug("App", "Loop");
    //     MatrixOS::USB::CDC::Println("Loop");
    //     MatrixOS::SYS::DelayMs(500);
    // }
}


int main()
{
    MatrixOS::SYS::Init();
    // Device::Bootloader();

    // Application(NULL);
    (void) xTaskCreateStatic(Application,"application",  APPLICATION_STACK_SIZE, NULL, configMAX_PRIORITIES-2, application_stack, &application_taskdef);

    #ifndef DONT_START_FREERTOS_SCHEDULER
    vTaskStartScheduler();
    #endif

    return 0;             
}