#include "Framework.h"
#include "Device.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "ff.h"
#include "diskio.h"

#include "MatrixOS.h"

namespace Device::FatFS
{
  static sdmmc_card_t* card = nullptr;
  static bool initialized = false;

  uint8_t Init(uint8_t pdrv)
  {

    // Initialize card detection pin if available
    if (sd_det_pin != GPIO_NUM_NC)
    {
      gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << sd_det_pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
      };
      gpio_config(&io_conf);
    }

    if (initialized && card != nullptr)
    {
      return 0; // Already initialized
    }

    esp_err_t ret;

    // SDMMC mode with configurable bus width
    if (sd_4bit_mode)
    {
      MLOGI("FatFS", "Initializing SDMMC 4-bit mode");
    }
    else
    {
      MLOGI("FatFS", "Initializing SDMMC 1-bit mode");
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = sd_freq_khz;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = sd_4bit_mode ? 4 : 1;
    slot_config.clk = sd_clk_pin;
    slot_config.cmd = sd_cmd_pin;
    slot_config.d0 = sd_d0_pin;

    if (sd_4bit_mode)
    {
      slot_config.d1 = sd_d1_pin;
      slot_config.d2 = sd_d2_pin;
      slot_config.d3 = sd_d3_pin;
    }

    // slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // Initialize SDMMC host
    ret = sdmmc_host_init();
    if (ret != ESP_OK)
    {
      MLOGE("FatFS", "Failed to init SDMMC host: %s", esp_err_to_name(ret));
      return STA_NOINIT;
    }

    ret = sdmmc_host_init_slot(host.slot, &slot_config);
    if (ret != ESP_OK)
    {
      MLOGE("FatFS", "Failed to init SDMMC slot: %s", esp_err_to_name(ret));
      sdmmc_host_deinit();
      return STA_NOINIT;
    }

    // Probe and initialize the card
    card = (sdmmc_card_t*)malloc(sizeof(sdmmc_card_t));
    ret = sdmmc_card_init(&host, card);
    if (ret != ESP_OK)
    {
      MLOGE("FatFS", "Failed to init card: %s", esp_err_to_name(ret));
      free(card);
      card = nullptr;
      sdmmc_host_deinit();
      return STA_NOINIT;
    }

    initialized = true;
    MLOGI("FatFS", "SD card initialized successfully");

    // Print card info
    // sdmmc_card_print_info(stdout, card);

    return 0; // Success
  }

  uint8_t Status(uint8_t pdrv)
  {
    if (!initialized || card == nullptr)
    {
      return STA_NOINIT;
    }

    // Check card detection pin (active high)
    if (sd_det_pin != GPIO_NUM_NC && gpio_get_level(sd_det_pin) == 0)
    {
      return STA_NODISK;
    }

    // Check if card is write protected (assume not for now)
    return 0; // Ready
  }

  uint8_t Read(uint8_t pdrv, uint8_t* buff, uint32_t sector, uint32_t count)
  {
    if (!initialized || card == nullptr)
    {
      return RES_NOTRDY;
    }

    esp_err_t err = sdmmc_read_sectors(card, buff, sector, count);
    if (err != ESP_OK)
    {
      MLOGE("FatFS", "Read sectors failed: %s", esp_err_to_name(err));
      return RES_ERROR;
    }

    return RES_OK;
  }

  uint8_t Write(uint8_t pdrv, const uint8_t* buff, uint32_t sector, uint32_t count)
  {
    if (!initialized || card == nullptr)
    {
      return RES_NOTRDY;
    }

    esp_err_t err = sdmmc_write_sectors(card, buff, sector, count);
    if (err != ESP_OK)
    {
      MLOGE("FatFS", "Write sectors failed: %s", esp_err_to_name(err));
      return RES_ERROR;
    }

    return RES_OK;
  }

  uint8_t IOControl(uint8_t pdrv, uint8_t cmd, void* buff)
  {
    if (!initialized || card == nullptr)
    {
      return RES_NOTRDY;
    }

    switch (cmd)
    {
      case CTRL_SYNC:
        // For SD cards, no explicit sync needed
        return RES_OK;

      case GET_SECTOR_COUNT:
        *(uint32_t*)buff = card->csd.capacity;
        return RES_OK;

      case GET_SECTOR_SIZE:
        *(uint16_t*)buff = card->csd.sector_size;
        return RES_OK;

      case GET_BLOCK_SIZE:
        *(uint32_t*)buff = card->csd.sector_size;
        return RES_OK;

      default:
        return RES_PARERR;
    }
  }
}