#include "EFuseBurnner.h"

void EFuseBurnner::Setup()
{
    if(esp_efuse_block_is_empty(EFUSE_BLK3))
    {
        UI efuseConfirm("eFuseConfirmation", Color(0xFFFFFF));

        efuseConfirm.AddUIElement(UIElement("Confirm", 
                                        Color(0x00FF00), 
                                        [&]() -> void {BurnEFuse(); Exit();}), 
                                        4, Point(1, 5), Point(2, 5), Point(1, 6), Point(2, 6));

        efuseConfirm.AddUIElement(UIElement("Cancel", 
                                    Color(0xFF0000), 
                                    [&]() -> void {Exit();}), 
                                    4, Point(5, 5), Point(6, 5), Point(5, 6), Point(6, 6));

        efuseConfirm.Start();
    }
    else
    {
        MatrixOS::LED::SetColor(Point(2,2), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(3,2), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(4,2), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(5,2), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(2,3), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(5,3), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(2,4), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(5,4), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(2,5), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(3,5), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(4,5), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(5,5), Color(0x00FF00));
        MatrixOS::LED::Update();
        MatrixOS::SYS::DelayMs(2000);
        Exit();
    }
}

void EFuseBurnner::BurnEFuse()
{
    MatrixOS::Logging::LogDebug("BurnEFuse", "Burning EFuse");
    MatrixOS::Logging::LogDebug("BurnEFuse", "Coding: %d", (int)esp_efuse_get_coding_scheme(EFUSE_BLK3));
    esp_efuse_batch_write_begin();

    esp_err_t status;

    // Disable USB Jtag (Enable Jtag via Pin) 
    status = esp_efuse_write_field_bit(ESP_EFUSE_DIS_USB_JTAG); 
    MatrixOS::Logging::LogDebug("BurnEFuse", "Burning EFUSE_DIS_USB_JTAG - Status: %s", esp_err_to_name(status));

    // Device Info
    DeviceInfo deviceInfo {{'M', 'X', '1', 'P'}, {'V', '1', '0', '0'}, 22, 05};
    status = esp_efuse_write_field_blob(ESP_EFUSE_USER_DATA, (void*)&deviceInfo, sizeof(deviceInfo) * 8);
    MatrixOS::Logging::LogDebug("BurnEFuse", "Burning Matrix Info - Status: %s", esp_err_to_name(status));

    // Write protection for DIS_ICACHE DIS_DCACHE DIS_DOWNLOAD_ICACHE DIS_DOWNLOAD_DCACHE DIS_FORCE_DOWNLOAD DIS_USB DIS_CAN SOFT_DIS_JTAG HARD_DIS_JTAG DIS_DOWNLOAD_MANUAL_ENCRYPT
    status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_GROUP_1); 
    MatrixOS::Logging::LogDebug("BurnEFuse", "Buriing EFUSE_WR_DIS_GROUP_1 - Status: %s", esp_err_to_name(status));

    // Write protection for VDD_SPI_XPD VDD_SPI_TIEH VDD_SPI_FORCE VDD_SPI_INIT VDD_SPI_DCAP WDT_DELAY_SEL
    status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_GROUP_2); 
    MatrixOS::Logging::LogDebug("BurnEFuse", "Buriing EFUSE_WR_DIS_GROUP_2 - Status: %s", esp_err_to_name(status));

    // Write protection for FLASH_TPUW DIS_DOWNLOAD_MODE DIS_LEGACY_SPI_BOOT UART_PRINT_CHANNEL DIS_USB_DOWNLOAD_MODE ENABLE_SECURITY_DOWNLOAD UART_PRINT_CONTROL PIN_POWER_SELECTION FLASH_TYPE FORCE_SEND_RESUME SECURE_VERSION,
    status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_GROUP_3);
    MatrixOS::Logging::LogDebug("BurnEFuse", "Buriing EFUSE_WR_DIS_GROUP_3 - Status: %s", esp_err_to_name(status));

    // Write protection for SPI_BOOT_CRYPT_CNT
    status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_SPI_BOOT_CRYPT_CNT);
    MatrixOS::Logging::LogDebug("BurnEFuse", "Buriing EFUSE_WR_DIS_SPI_BOOT_CRYPT_CNT - Status: %s", esp_err_to_name(status));

    // Write protection for USB_EXCHG_PINS,
    status = esp_efuse_write_field_bit(ESP_EFUSE_WR_DIS_USB_EXCHG_PINS);
    MatrixOS::Logging::LogDebug("BurnEFuse", "Buriing ESP_EFUSE_WR_DIS_USB_EXCHG_PINS - Status: %s", esp_err_to_name(status));

    // Write protection for Matrix Info
    status = esp_efuse_set_write_protect(EFUSE_BLK3);
    MatrixOS::Logging::LogDebug("BurnEFuse", "Block 3 (Matrix Info) write protected - Status: %s", esp_err_to_name(status));
    esp_efuse_batch_write_commit();
}