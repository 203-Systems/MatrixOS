#include "MatrixOS.h"
#include "USB.h"

#include "tusb.h"

namespace MatrixOS::USB
{
static constexpr char TAG[] = "USB";
static uint8_t mode = USB_MODE_NORMAL;

// Function to get current mode for descriptor callbacks
uint8_t GetMode() {
  return mode;
}

void usb_device_task(void* param) {
  (void)param;
  // RTOS forever loop
  while (1)
  {
    // tinyusb device task
    tud_task();
  }
}

// Create a task for tinyusb device stack
#define USBD_STACK_SIZE (3 * configMINIMAL_STACK_SIZE)
StackType_t usb_device_stack[USBD_STACK_SIZE];
StaticTask_t usb_device_taskdef;
void Init(USB_MODE mode) {
  MatrixOS::USB::mode = mode;
  tusb_init();
  (void)xTaskCreateStatic(usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, usb_device_stack, &usb_device_taskdef);
  if (mode == USB_MODE_NORMAL)
  {
    USB::MIDI::Init();
  }
}

bool Disconnect() {
  return tud_disconnect();
}

bool Connect() {
  return tud_connect();
}

bool Connected() {
  return tud_ready();
}

uint16_t GetBCDID() {
  return MATRIXOS_VERSION_ID_16;
}

void SetMode(USB_MODE newMode) {
  if (newMode != USB_MODE_NORMAL
#if DEVICE_STORAGE
      && newMode != USB_MODE_MSC
#endif
  )
  {
    MLOGE(TAG, "Unsupported USB mode: %u", newMode);
    return;
  }
  if (mode == newMode)
  {
    return;
  }
  if (!tud_inited())
  {
    mode = newMode;
    return;
  }

  const uint8_t previousMode = mode;
  MLOGI(TAG, "Switching mode %u -> %u (mounted=%u, suspended=%u)", previousMode, newMode, tud_mounted(), tud_suspended());

  // Suspended devices still need to disconnect before changing descriptors.
  if (!Disconnect())
  {
    MLOGE(TAG, "Failed to disconnect for mode switch");
    return;
  }

  // Let the host detect removal and drain requests for the old configuration.
  vTaskDelay(pdMS_TO_TICKS(250));
  mode = newMode;
  if (!Connect())
  {
    MLOGE(TAG, "Failed to connect in mode %u", newMode);
    mode = previousMode;
    if (!Connect())
    {
      MLOGE(TAG, "Failed to restore USB mode %u", previousMode);
    }
  }
}

} // namespace MatrixOS::USB

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {}

// Invoked when device is unmounted
void tud_umount_cb(void) {}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {}
