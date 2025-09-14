#include "Device.h"
#include "Storage.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"
#include "sdmmc_cmd.h"

namespace Device
{
  namespace Storage
  {
    static sdmmc_card_t card;
    static bool initialized = false;
    static bool card_failed = false; // Card present but failed to initialize
    static Status status = {0}; // Initialize all fields to 0/false

    void InitCard()
    {
      esp_err_t ret;

      // Initialize SDMMC host
      sdmmc_host_t host = SDMMC_HOST_DEFAULT();
      if (!Device::Storage::sd_4bit_mode)
      {
        host.flags = SDMMC_HOST_FLAG_1BIT;
      }
      host.max_freq_khz = Device::Storage::sd_freq_khz;

      // Initialize slot with custom pins
      sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
      slot_config.width = Device::Storage::sd_4bit_mode ? 4 : 1;
      slot_config.cd = SDMMC_SLOT_NO_CD; // We handle card detect ourselves
      slot_config.wp = SDMMC_SLOT_NO_WP;
      slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

      // Configure custom pins
      slot_config.clk = Device::Storage::sd_clk_pin;
      slot_config.cmd = Device::Storage::sd_cmd_pin;
      slot_config.d0 = Device::Storage::sd_d0_pin;
      if (Device::Storage::sd_4bit_mode) {
        slot_config.d1 = Device::Storage::sd_d1_pin;
        slot_config.d2 = Device::Storage::sd_d2_pin;
        slot_config.d3 = Device::Storage::sd_d3_pin;
      }

      ret = sdmmc_host_init();
      if (ret != ESP_OK)
      {
        MLOGE("Storage", "Failed to init SDMMC host: %s", esp_err_to_name(ret));
        return;
      }

      ret = sdmmc_host_init_slot(SDMMC_HOST_SLOT_1, &slot_config);
      if (ret != ESP_OK)
      {
        MLOGE("Storage", "Failed to init SDMMC slot: %s", esp_err_to_name(ret));
        sdmmc_host_deinit();
        return;
      }

      // Initialize the card
      ret = sdmmc_card_init(&host, &card);
      if (ret != ESP_OK)
      {
        MLOGD("Storage", "Card init failed: %s", esp_err_to_name(ret));
        sdmmc_host_deinit();
        card_failed = true; // Mark card as failed, don't retry
        return;
      }

      MLOGD("Storage", "SD card initialized successfully");
      initialized = true;
      card_failed = false; // Clear any previous failure
    }

    void DeinitCard()
    {
      if (initialized)
      {
        sdmmc_host_deinit();
        initialized = false;
        MLOGD("Storage", "SD card deinitialized");
      }
      card_failed = false; // Reset failure state on card removal
    }

    void Init()
    {
      // Configure GPIO pin once
      if (Device::Storage::sd_det_pin != GPIO_NUM_NC)
      {
        gpio_config_t io_conf = {
          .pin_bit_mask = (1ULL << Device::Storage::sd_det_pin),
          .mode = GPIO_MODE_INPUT,
          .pull_up_en = GPIO_PULLUP_DISABLE,
          .pull_down_en = GPIO_PULLDOWN_ENABLE,
          .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
      }

      GetStatus();
    }

    const Status* GetStatus()
    {

      // Check if card is present (active high detection)
      bool card_present = true;
      if (Device::Storage::sd_det_pin != GPIO_NUM_NC)
      {
        card_present = (gpio_get_level(Device::Storage::sd_det_pin) == 1);
      }

      // Check if card state changed
      if (card_present && !initialized && !card_failed)
      {
        // Card present and not initialized and not failed -> try to init
        InitCard();
      }
      else if (!card_present && (initialized || card_failed))
      {
        // Card removed -> deinit and reset states
        DeinitCard();
      }
      else if (Device::Storage::sd_det_pin == GPIO_NUM_NC && initialized)
      {
        // No card detect pin - check if ESP-IDF detects card removal
        // This can happen when user removes card during operation
        // We could add a simple read test here if needed for detection
      }

      // Update status based on current state
      if (initialized && card_present)
      {
        // Card available - populate status
        status.available = true;
        status.sector_count = card.csd.capacity;
        status.sector_size = 512;

        uint32_t erase_block_sectors = card.ssr.erase_size_au * card.ssr.alloc_unit_kb * (1024 / 512);
        status.block_size = (erase_block_sectors > 0) ? erase_block_sectors : 1;

        status.write_protected = false;
      }
      else
      {
        // Card not available - reset status
        status.available = false;
        status.sector_count = 0;
        status.sector_size = 512;
        status.block_size = 1;
        status.write_protected = false;
      }

      return &status;
    }


    bool ReadSectors(uint8_t pdrv, uint32_t lba, uint8_t* buffer, uint32_t count)
    {
      const Status* s = GetStatus();

      if (!s->available)
      {
        return false; // Error
      }

      esp_err_t ret = sdmmc_read_sectors(&card, buffer, lba, count);
      return (ret == ESP_OK);
    }

    bool WriteSectors(uint8_t pdrv, uint32_t lba, const uint8_t* buffer, uint32_t count)
    {
      const Status* s = GetStatus();

      if (!s->available || s->write_protected)
      {
        return false; // Error
      }

      esp_err_t ret = sdmmc_write_sectors(&card, buffer, lba, count);
      return (ret == ESP_OK);
    }
  }
}