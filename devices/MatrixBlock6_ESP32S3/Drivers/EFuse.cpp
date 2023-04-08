#include "Device.h"

#ifdef FACTORY_CONFIG
#include "esp_efuse.h"
#include "esp_efuse_table.h"
void BurnEFuse() {
  MLOGD("BurnEFuse", "Burning EFuse");
  MLOGD("BurnEFuse", "Coding: %d", (int)esp_efuse_get_coding_scheme(EFUSE_BLK3));
  esp_efuse_batch_write_begin();

  esp_err_t status;

  // Device Info
  status = esp_efuse_write_field_blob(ESP_EFUSE_USER_DATA, (void*)&Device::deviceInfo, sizeof(DeviceInfo) * 8);
  MLOGD("BurnEFuse", "Burning Matrix Info - Status: %s", esp_err_to_name(status));

  // Disable USB Jtag (Enable Jtag via Pin)
  status = esp_efuse_write_field_bit(ESP_EFUSE_DIS_USB_JTAG);
  MLOGD("BurnEFuse", "Burning EFUSE_DIS_USB_JTAG - Status: %s", esp_err_to_name(status));

  // Write protection for DIS_ICACHE DIS_DCACHE DIS_DOWNLOAD_ICACHE DIS_DOWNLOAD_DCACHE DIS_FORCE_DOWNLOAD DIS_USB
  // DIS_CAN SOFT_DIS_JTAG HARD_DIS_JTAG DIS_DOWNLOAD_MANUAL_ENCRYPT
  status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_GROUP_1);
  MLOGD("BurnEFuse", "Buriing EFUSE_WR_DIS_GROUP_1 - Status: %s", esp_err_to_name(status));

  // Write protection for VDD_SPI_XPD VDD_SPI_TIEH VDD_SPI_FORCE VDD_SPI_INIT VDD_SPI_DCAP WDT_DELAY_SEL
  status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_GROUP_2);
  MLOGD("BurnEFuse", "Buriing EFUSE_WR_DIS_GROUP_2 - Status: %s", esp_err_to_name(status));

  // Write protection for FLASH_TPUW DIS_DOWNLOAD_MODE DIS_LEGACY_SPI_BOOT UART_PRINT_CHANNEL DIS_USB_DOWNLOAD_MODE
  // ENABLE_SECURITY_DOWNLOAD UART_PRINT_CONTROL PIN_POWER_SELECTION FLASH_TYPE FORCE_SEND_RESUME SECURE_VERSION,
  status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_GROUP_3);
  MLOGD("BurnEFuse", "Buriing EFUSE_WR_DIS_GROUP_3 - Status: %s", esp_err_to_name(status));

  // Write protection for SPI_BOOT_CRYPT_CNT
  status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_SPI_BOOT_CRYPT_CNT);
  MLOGD("BurnEFuse", "Buriing EFUSE_WR_DIS_SPI_BOOT_CRYPT_CNT - Status: %s",
                              esp_err_to_name(status));

  // Write protection for USB_EXCHG_PINS,
  status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_USB_EXCHG_PINS);
  MLOGD("BurnEFuse", "Buriing ESP_EFUSE_WR_DIS_USB_EXCHG_PINS - Status: %s",
                              esp_err_to_name(status));

  // Write protection for Matrix Info
  status = esp_efuse_set_write_protect(EFUSE_BLK3);
  MLOGD("BurnEFuse", "Block 3 (Matrix Info) write protected - Status: %s",
                              esp_err_to_name(status));
  esp_efuse_batch_write_commit();
}

#endif