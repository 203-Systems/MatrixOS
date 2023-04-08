#include "MatrixOS.h"
#include "MIDI.h"

namespace MatrixOS::USB
{
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
  void Init() {
    tusb_init();
    (void)xTaskCreateStatic(usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, usb_device_stack,
                            &usb_device_taskdef);
    USB::MIDI::Init();
  }

  bool Inited() {
    return tusb_inited();
  }

  bool Connected() {
    return tud_ready();
  }

  uint16_t GetBCDID() {
    return ((USB_CDC_COUNT & 0b11) << 14) + ((USB_MIDI_COUNT & 0b11) << 12) + ((USB_HID_COUNT & 0b11) << 10) +
           ((USB_MSC_COUNT & 0b11) << 8) + ((USB_VENDOR_COUNT & 0b11) << 6) + (MatrixOS::UserVar::device_id & 0b111111);
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
