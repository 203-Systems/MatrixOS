#include "MatrixOS.h"
#include "USB.h"

#include "tusb.h"

namespace MatrixOS::USB
{
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
    (void)xTaskCreateStatic(usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, usb_device_stack,
                            &usb_device_taskdef);
    if (mode == USB_MODE_NORMAL) {
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


  // New unified SetMode API
  void SetMode(USB_MODE new_mode) {
    if (mode != new_mode) {
      mode = new_mode;
      if (Connected()) {
        Disconnect();
        vTaskDelay(pdMS_TO_TICKS(100));
        Connect();
      }
    }
  }

}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{

}

// Invoked when device is unmounted
void tud_umount_cb(void)
{

}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{

}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{

}
