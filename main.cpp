#include "MatrixOS.h"
#include "applications/Applications.h"

void Application(void* param)
{
    // NVSTest nvsTest;
    // nvsTest.Start();
    Performance performance;
    performance.Start();

    // TestApp TestApp;
    // TestApp.Start();

    // REDACTED redacted;
    // redacted.Start();

    // WirelessRepeater wirelessRepeater;
    // wirelessRepeater.Start();

    // while(true){ 
    //     MatrixOS::SYS::DelayMs(10);
    // }
}

StackType_t  application_stack[APPLICATION_STACK_SIZE];
StaticTask_t application_taskdef;

int main()
{
    MatrixOS::SYS::Init();

    (void) xTaskCreateStatic(Application,"application",  APPLICATION_STACK_SIZE, NULL, 1, application_stack, &application_taskdef);
    #ifndef ESP_IDF_VERSION //ESP32s Doesn't need to start scheduler
    vTaskStartScheduler();
    #else
    vTaskDelete(NULL);
    #endif

    return 0;             
}