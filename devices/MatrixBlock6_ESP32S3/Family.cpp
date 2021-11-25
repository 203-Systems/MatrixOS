#include "Device.h"

namespace Device
{
    void DeviceInit()
    {
        USB_Init();
        LED_Init();
        NVS_Init();
    }


    // bool wdt_subscribed = false;
    void DeviceTask()
    {   
        // if(wdt_subscribed == false)
        // {
        //     esp_task_wdt_add(NULL);
        //     esp_task_wdt_status(NULL);
        // }

        // ESP_LOGI("Device", "Task Watchdog Reset1");
        // esp_task_wdt_reset();
        // ESP_LOGI("Device", "Task Watchdog Reset2");
    }

    void Bootloader()
    {
        // Check out esp_reset_reason_t for other Espressif pre-defined values
        #define APP_REQUEST_UF2_RESET_HINT (esp_reset_reason_t)0x11F2

        // call esp_reset_reason() is required for idf.py to properly links esp_reset_reason_set_hint()
        (void) esp_reset_reason();
        esp_reset_reason_set_hint(APP_REQUEST_UF2_RESET_HINT);
        esp_restart();
    }

    void Reboot()
    {
        esp_restart();
    }

    void Delay(uint32_t interval)
    {
        vTaskDelay(pdMS_TO_TICKS(interval));
    }

    uint32_t Millis()
    {
        return ((((uint64_t) xTaskGetTickCount()) * 1000) / configTICK_RATE_HZ );
        // return 0;
    }

    void ErrorHandler()
    {
        
    }

    void Log(ELogLevel level, string tag, string format, ...)
    {
        // va_list valst;
        // ESP_LOG_LEVEL((esp_log_level_t)level, tag.c_str(), format.c_str(), valst);
        // esp_log_write((esp_log_level_t)level, tag.c_str(), LOG_FORMAT(I, format.c_str()), esp_log_timestamp(), tag.c_str(), valst);
        // esp_log_write(ESP_LOG_INFO, tag.c_str(), LOG_COLOR_I "I"" (%u) %s: " format.c_str() LOG_RESET_COLOR "\n", esp_log_timestamp(), tag.c_str(), valst); 
        // do { 
        //     if (level==ESP_LOG_ERROR ) 
        //     { 
        //         esp_log_write(ESP_LOG_ERROR, tag.c_str(), LOG_COLOR_E "E"" (%u) %s: " format.c_str() LOG_RESET_COLOR "\n", esp_log_timestamp(), tag.c_str(), valst); 
        //     } else if (level==ESP_LOG_WARN ) { 
        //         esp_log_write(ESP_LOG_WARN, tag.c_str(), LOG_COLOR_W "W"" (%u) %s: " format.c_str() LOG_RESET_COLOR "\n", esp_log_timestamp(), tag.c_str(), valst); 
        //     } else if (level==ESP_LOG_DEBUG ) { 
        //         esp_log_write(ESP_LOG_DEBUG, tag.c_str(), LOG_COLOR_D "D"" (%u) %s: " format.c_str() LOG_RESET_COLOR "\n", esp_log_timestamp(), tag.c_str(), valst); 
        //     } else if (level==ESP_LOG_VERBOSE ) { 
        //         esp_log_write(ESP_LOG_VERBOSE, tag.c_str(), LOG_COLOR_V "V"" (%u) %s: " format.c_str() LOG_RESET_COLOR "\n", esp_log_timestamp(), tag.c_str(), valst); 
        //     } else { 
        //         esp_log_write(ESP_LOG_INFO, tag.c_str(), LOG_COLOR_I "I"" (%u) %s: " format.c_str() LOG_RESET_COLOR "\n", esp_log_timestamp(), tag.c_str(), valst); 
        //     } 
        // } while(0);

    }

    string GetSerial()
    {
        return "<Serial Number>"; //TODO
    }
}

namespace MatrixOS::SYS
{
    void ErrorHandler(char const* error);
}

extern "C"
{
    int main();
    void app_main(void)
    {
        main();
    }
}
