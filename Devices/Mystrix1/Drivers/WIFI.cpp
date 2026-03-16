// #include "Device.h"

// namespace Device::WIFI
// {
//   void Init(void) {
//     ESP_ERROR_CHECK(esp_netif_init());
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     cfg.ampdu_tx_enable = 0;
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
//     ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
//     ESP_ERROR_CHECK(esp_wifi_start());
//   }
// }