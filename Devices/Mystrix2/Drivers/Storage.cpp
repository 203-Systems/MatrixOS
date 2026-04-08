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
static bool cardFailed = false;   // Card present but failed to initialize
static StorageStatus status = {0}; // Initialize all fields to 0/false

// Forward declaration
bool Available();

// GPIO interrupt handler for card detect
static void IRAM_ATTR cardDetectIsrHandler(void* arg) {
  if (gpio_get_level(Device::Storage::sdDetPin) == 0)
  {
    // Card removed - reset status immediately
    status.available = false;
    status.sectorCount = 0;
    status.sectorSize = 0;
    status.blockSize = 0;
    status.writeProtected = false;
  }
}

void InitCard() {
  esp_err_t ret;

  // Initialize SDMMC host
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  if (!Device::Storage::sd4BitMode)
  {
    host.flags = SDMMC_HOST_FLAG_1BIT;
  }
  host.max_freq_khz = Device::Storage::sdFreqKhz;

  // Initialize slot with custom pins
  sdmmc_slot_config_t slotConfig = SDMMC_SLOT_CONFIG_DEFAULT();
  slotConfig.width = Device::Storage::sd4BitMode ? 4 : 1;
  slotConfig.cd = SDMMC_SLOT_NO_CD; // We handle card detect ourselves
  slotConfig.wp = SDMMC_SLOT_NO_WP;
  slotConfig.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  // Configure custom pins
  slotConfig.clk = Device::Storage::sdClkPin;
  slotConfig.cmd = Device::Storage::sdCmdPin;
  slotConfig.d0 = Device::Storage::sdD0Pin;
  if (Device::Storage::sd4BitMode)
  {
    slotConfig.d1 = Device::Storage::sdD1Pin;
    slotConfig.d2 = Device::Storage::sdD2Pin;
    slotConfig.d3 = Device::Storage::sdD3Pin;
  }

  ret = sdmmc_host_init();
  if (ret != ESP_OK)
  {
    return;
  }

  ret = sdmmc_host_init_slot(SDMMC_HOST_SLOT_1, &slotConfig);
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
    cardFailed = true; // Mark card as failed, don't retry
    return;
  }

  initialized = true;
  cardFailed = false; // Clear any previous failure
}

void DeinitCard() {
  if (initialized)
  {
    sdmmc_host_deinit();
    initialized = false;
  }
  cardFailed = false; // Reset failure state on card removal
}

void Init() {
  // Configure GPIO pin once
  if (Device::Storage::sdDetPin != GPIO_NUM_NC)
  {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << Device::Storage::sdDetPin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE // Trigger on both rising and falling edges
    };
    gpio_config(&io_conf);

    // Install GPIO ISR service if not already installed
    gpio_install_isr_service(0);

    // Add ISR handler for card detect pin
    gpio_isr_handler_add(Device::Storage::sdDetPin, cardDetectIsrHandler, NULL);
  }

  Available(); // Initial check
}

bool Available() {
  // Check if card is present (active high detection)
  bool cardPresent = true;
  if (Device::Storage::sdDetPin != GPIO_NUM_NC)
  {
    cardPresent = (gpio_get_level(Device::Storage::sdDetPin) == 1);
  }

  // if(cardPresent && initialized)
  // {
  //     uint8_t testBuffer[512];
  //     esp_err_t testResult = sdmmc_read_sectors(&card, testBuffer, 0, 1);
  //     if (testResult != ESP_OK) {
  //       // Card not responding - mark as failed and deinit
  //       cardPresent = false;
  //     }
  //   }
  // }

  // Check if card state changed
  if (cardPresent && !initialized && !cardFailed)
  {
    // Card present and not initialized and not failed -> try to init
    InitCard();
  }
  else if (!cardPresent && initialized)
  {
    // Card removed -> deinit and reset states
    DeinitCard();
  }

  // Update status based on current state
  if (initialized && cardPresent)
  {
    // Card available - populate status
    status.available = true;
    status.sectorCount = card.csd.capacity;
    status.sectorSize = card.csd.sector_size;
    status.blockSize = card.csd.sector_size;
    status.writeProtected = false;
  }
  else
  {
    // Card not available - reset status
    status.available = false;
    status.sectorCount = 0;
    status.sectorSize = 0;
    status.blockSize = 0;
    status.writeProtected = false;
  }

  return status.available;
}

const StorageStatus* Status() {
  return &status;
}

bool ReadSectors(uint32_t lba, uint32_t sectorCount, void* dest) {
  if (!status.available)
  {
    return false; // Error
  }

  esp_err_t ret = sdmmc_read_sectors(&card, dest, lba, sectorCount);
  return (ret == ESP_OK);
}

bool WriteSectors(uint32_t lba, uint32_t sectorCount, const void* src) {
  if (!status.available || status.writeProtected)
  {
    return false; // Error
  }

  esp_err_t ret = sdmmc_write_sectors(&card, src, lba, sectorCount);
  return (ret == ESP_OK);
}
} // namespace Storage
} // namespace Device