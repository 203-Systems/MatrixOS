#include "Device.h"
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
    static StorageStatus status = {0}; // Initialize all fields to 0/false

    // Forward declaration
    bool Available();

    // GPIO interrupt handler for card detect
    static void IRAM_ATTR card_detect_isr_handler(void* arg)
    {
      if (gpio_get_level(Device::Storage::sd_det_pin) == 0) {
        // Card removed - reset status immediately
        status.available = false;
        status.sector_count = 0;
        status.sector_size = 0;
        status.block_size = 0;
        status.write_protected = false;
      }
    }

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
        return;
      }

      ret = sdmmc_host_init_slot(SDMMC_HOST_SLOT_1, &slot_config);
      if (ret != ESP_OK)
      {
        sdmmc_host_deinit();
        return;
      }

      // Initialize the card
      ret = sdmmc_card_init(&host, &card);
      if (ret != ESP_OK)
      {
        sdmmc_host_deinit();
        card_failed = true; // Mark card as failed, don't retry
        return;
      }

      initialized = true;
      card_failed = false; // Clear any previous failure
    }

    void DeinitCard()
    {
      if (initialized)
      {
        sdmmc_host_deinit();
        initialized = false;
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
          .intr_type = GPIO_INTR_ANYEDGE  // Trigger on both rising and falling edges
        };
        gpio_config(&io_conf);

        // Install GPIO ISR service if not already installed
        gpio_install_isr_service(0);

        // Add ISR handler for card detect pin
        gpio_isr_handler_add(Device::Storage::sd_det_pin, card_detect_isr_handler, NULL);
      }

      Available();  // Initial check
    }

    bool Available()
    {
      // Check if card is present (active high detection)
      bool card_present = true;
      if (Device::Storage::sd_det_pin != GPIO_NUM_NC)
      {
        card_present = (gpio_get_level(Device::Storage::sd_det_pin) == 1);
      }
      
      // if(card_present && initialized)
      // {
      //     uint8_t test_buffer[512];
      //     esp_err_t test_result = sdmmc_read_sectors(&card, test_buffer, 0, 1);
      //     if (test_result != ESP_OK) {
      //       // Card not responding - mark as failed and deinit
      //       card_present = false;
      //     }
      //   }
      // }

      // Check if card state changed
      if (card_present && !initialized && !card_failed)
      {
        // Card present and not initialized and not failed -> try to init
        InitCard();
      }
      else if (!card_present && initialized)
      {
        // Card removed -> deinit and reset states
        DeinitCard();
      }

      // Update status based on current state
      if (initialized && card_present)
      {
        // Card available - populate status
        status.available = true;
        status.sector_count = card.csd.capacity;
        status.sector_size = card.csd.sector_size;
        status.block_size = card.csd.sector_size;
        status.write_protected = false;
      }
      else
      {
        // Card not available - reset status
        status.available = false;
        status.sector_count = 0;
        status.sector_size = 0;
        status.block_size = 0;
        status.write_protected = false;
      }

      return status.available;
    }

    const StorageStatus* Status()
    {
      return &status;
    }


    bool ReadSectors(uint32_t lba, uint32_t sector_count, void* dest)
    {
      if (!status.available)
      {
        return false; // Error
      }

      esp_err_t ret = sdmmc_read_sectors(&card, dest, lba, sector_count);
      return (ret == ESP_OK);
    }

    bool WriteSectors(uint32_t lba, uint32_t sector_count, const void* src)
    {
      if (!status.available || status.write_protected)
      {
        return false; // Error
      }

      esp_err_t ret = sdmmc_write_sectors(&card, src, lba, sector_count);
      return (ret == ESP_OK);
    }
  }
}