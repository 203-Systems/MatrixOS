#include "MatrixOS.h"
#include "application/Applications.h"


#define APP_STACK_SIZE     4096
void Application(void* param)
{
    TestApp TestApp;
    TestApp.Start();
}

int main()
{
    MatrixOS::SYS::Init();

    // Create a task for tinyusb device stack

    // while (true)
    // {
    //     MatrixOS::SYS::SystemTask();
    // }

    // EEPROMTest eepromTest;
    // eepromTest.Start();
    Performance performance;
    performance.Start();

    // TestApp TestApp;
    // TestApp.Start();

    // (void) xTaskCreate(Application,"application", APP_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL);

    // while(1)
    // {
    //   ESP_LOGI("Device", "Task Watchdog Reset1");
    //   esp_task_wdt_reset();
    // }

    return 0;             
}