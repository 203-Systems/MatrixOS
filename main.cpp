#include "MatrixOS.h"
#include "application/Applications.h"


#define APPLICATION_STACK_SIZE     (configMINIMAL_STACK_SIZE * 8)
StackType_t  application_stack[APPLICATION_STACK_SIZE];
StaticTask_t application_taskdef;
void Application(void* param)
{
    // NVSTest nvsTest;
    // nvsTest.Start();
    Performance performance;
    performance.Start();

    // REDACTED redacted;
    // redacted.Start();

    // TestApp TestApp;
    // TestApp.Start();

    // while(true){
    //     MatrixOS::SYS::DelayMs(10);
    // }
}


int main()
{
    MatrixOS::SYS::Init();

    // MatrixOS::Logging::LogDebug("main", "Adding this will crazsh the program, comment it out then it will be fine");

    // Application(NULL);
    (void) xTaskCreateStatic(Application,"application",  APPLICATION_STACK_SIZE, NULL, configMAX_PRIORITIES-1, application_stack, &application_taskdef);

    #ifndef DONT_START_FREERTOS_SCHEDULER
    vTaskStartScheduler();
    #endif

    return 0;             
}