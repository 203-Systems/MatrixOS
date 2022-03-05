#include "MatrixOS.h"
#include "applications/Applications.h"

void Application(void* param)
{
    // NVSTest nvsTest;
    // nvsTest.Start();
    // Performance performance;
    // performance.Start();

    // TestApp TestApp;
    // TestApp.Start();

    REDACTED redacted;
    redacted.Start();

    // WirelessRepeater wirelessRepeater;
    // wirelessRepeater.Start();

    // while(true){ 
    //     MatrixOS::SYS::DelayMs(10);
    // }
}

#define APPLICATION_STACK_SIZE     (configMINIMAL_STACK_SIZE * 4)
int main()
{
    MatrixOS::SYS::Init();

    #ifndef ESP_IDF_VERSION //ESP32s Doesn't need to start scheduler
    StackType_t  application_stack[APPLICATION_STACK_SIZE];
    StaticTask_t application_taskdef;
    (void) xTaskCreateStatic(Application,"application",  APPLICATION_STACK_SIZE, NULL, configMAX_PRIORITIES-1, application_stack, &application_taskdef);
    vTaskStartScheduler();
    #else
    Application(NULL);
    #endif

    return 0;             
}