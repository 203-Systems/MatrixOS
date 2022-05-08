// #pragma once

// #include "esp_efuse.h"
// #include "esp_efuse_table.h"
// // #ifdef FACTORY

// inline void BurnEFuse()
// {
//     // Disable USB Jtag (Enable Jtag via Pin) 
//     esp_efuse_write_field_bit(ESP_EFUSE_DIS_USB_JTAG); 

//     // Device Info
//     DeviceInfo deviceInfo {{'M', 'X', '1', 'P'}, {'V', '1', '0', '0'}, 22, 05};
//     esp_efuse_write_field_blob(ESP_EFUSE_USER_DATA, (void*)&deviceInfo, sizeof(deviceInfo)* 8);

// }
// // #endif