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

    // WirelessRepeater wirelessRepeater;
    // wirelessRepeater.Start();

    // while(true){ 
    //     MatrixOS::SYS::DelayMs(10);
    // }
}


int main()
{
    MatrixOS::SYS::Init();

    #ifndef ESP_IDF_VERSION //ESP32s Doesn't need to start scheduler
    (void) xTaskCreateStatic(Application,"application",  APPLICATION_STACK_SIZE, NULL, configMAX_PRIORITIES-1, application_stack, &application_taskdef);
    vTaskStartScheduler();
    #else
    Application(NULL);
    #endif

    return 0;             
}