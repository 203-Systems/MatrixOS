#include "Device.h"
#include "MatrixOS.h"
#include "ui/UI.h"

namespace Device
{
  namespace KeyPad
  {
    void Scan();
  }

  void DeviceInit() {
    // esp_timer_early_init();
    LoadDeviceInfo();
    gpio_install_isr_service(0);
    USB::Init();
    NVS::Init();
    LED::Init();
    KeyPad::Init();
  }

  void DeviceStart() {
    Device::KeyPad::Start();
    Device::LED::Start();
  }

  void LoadDeviceInfo() {
#ifndef FACTORY_CONFIG
    esp_efuse_read_field_blob(ESP_EFUSE_USER_DATA, &deviceInfo, sizeof(deviceInfo) * 8);
#endif
    LoadVarientInfo();
  }

  void Bootloader() {
// Check out esp_reset_reason_t for other Espressif pre-defined values
#define APP_REQUEST_UF2_RESET_HINT (esp_reset_reason_t)0x11F2

    // call esp_reset_reason() is required for idf.py to properly links esp_reset_reason_set_hint()
    (void)esp_reset_reason();
    esp_reset_reason_set_hint(APP_REQUEST_UF2_RESET_HINT);
    esp_restart();
  }

  void Reboot() {
    esp_restart();
  }

  void Delay(uint32_t interval) {
    vTaskDelay(pdMS_TO_TICKS(interval));
  }

  uint32_t Millis() {
    return ((((uint64_t)xTaskGetTickCount()) * 1000) / configTICK_RATE_HZ);
    // return 0;
  }

  void Log(string format, va_list valst) {
    // ESP_LOG_LEVEL((esp_log_level_t)level, tag.c_str(), format.c_str(), valst);
    // esp_log_writev(ESP_LOG_INFO, format.c_str(), valst);

    vprintf(format.c_str(), valst);
  }

  string GetSerial() {
    uint8_t uuid[16];
    esp_efuse_read_field_blob(ESP_EFUSE_OPTIONAL_UNIQUE_ID, (void*)uuid, 128);
    string uuid_str;
    uuid_str.reserve(33);
    const char char_table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    for (uint8_t i = 0; i < 16; i++)
    {
      uuid_str += char_table[uuid[i] >> 4];
      uuid_str += char_table[uuid[i] & 0x0F];
    }
    return uuid_str;
  }

  void ErrorHandler() {}
}

extern "C" {
  int main();
  void app_main(void) {
    main();
  }
}
