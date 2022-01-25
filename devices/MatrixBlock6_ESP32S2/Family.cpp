#include "Device.h"

namespace Device
{
    void DeviceInit()
    {
        USB::Init();
        LED::Init();
        KeyPad::Init();
        NVS::Init();
        WIFI::Init();
        ESPNOW::Init();
        ESPNOW::BroadcastMac();
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

    void Log(string format, va_list valst)
    {
        // ESP_LOG_LEVEL((esp_log_level_t)level, tag.c_str(), format.c_str(), valst);
        // esp_log_writev(ESP_LOG_INFO, format.c_str(), valst);
        
        vprintf(format.c_str(), valst);
    }

    string GetSerial()
    {
        return "<Serial Number>"; //TODO
    }

    
    void ErrorHandler()
    {
        
    }

    namespace MIDI
    {
        uint32_t Available()
        {
            uint32_t packets = 0;
            packets += ESPNOW::MidiAvailable();
            return packets;
        }

        MidiPacket Get()
        {
            if(ESPNOW::MidiAvailable())
            {
                return ESPNOW::GetMidi();
            }
            return MidiPacket(0, None);
        }

        bool Sent(MidiPacket packet)
        {
            return ESPNOW::SendMidi(packet.data);
        }
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
