#include "MatrixOS.h"
#include "application/Applications.h"


#define APP_STACK_SIZE     8196
void Application(void* param)
{
    // NVSTest nvsTest;
    // nvsTest.Start();
    Performance performance;
    performance.Start();

    // TestApp TestApp;
    // TestApp.Start();
}


int main()
{
    MatrixOS::SYS::Init();

    (void) xTaskCreate(Application,"application", APP_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);

    // while(1)
    // {
    //   ESP_LOGI("Device", "Task Watchdog Reset1");
    //   esp_task_wdt_reset();
    // }

    // (void) xTaskCreateStatic( usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES-1, usb_device_stack, &usb_device_taskdef);


    #ifndef DONT_START_FREERTOS_SCHEDULER
    vTaskStartScheduler();
    #endif

    return 0;             
}