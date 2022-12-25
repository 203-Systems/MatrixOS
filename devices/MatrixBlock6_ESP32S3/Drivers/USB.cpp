#include "Device.h"

#include "esp_private/periph_ctrl.h"
#if CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/gpio.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/gpio.h"
#endif

namespace Device
{
  namespace USB
  {
    void Init() {
      // USB Controller Hal init
      periph_module_reset(PERIPH_USB_MODULE);
      periph_module_enable(PERIPH_USB_MODULE);

      usb_hal_context_t hal = {
          .use_external_phy = false  // use built-in PHY
      };
      usb_hal_init(&hal);

      /* usb_periph_iopins currently configures USB_OTG as USB Device.
       * Introduce additional parameters in usb_hal_context_t when adding support
       * for USB Host.
       */
      for (const usb_iopin_dsc_t* iopin = usb_periph_iopins; iopin->pin != -1; ++iopin)
      {
        if ((hal.use_external_phy) || (iopin->ext_phy_only == 0))
        {
          esp_rom_gpio_pad_select_gpio((uint32_t)iopin->pin);
          if (iopin->is_output)
          { esp_rom_gpio_connect_out_signal(iopin->pin, iopin->func, false, false); }
          else
          {
            esp_rom_gpio_connect_in_signal(iopin->pin, iopin->func, false);
            if ((iopin->pin != GPIO_FUNC_IN_LOW) && (iopin->pin != GPIO_FUNC_IN_HIGH))
            { gpio_ll_input_enable(&GPIO, (gpio_num_t)iopin->pin); }
          }
          esp_rom_gpio_pad_unhold(iopin->pin);
        }
      }

      if (!hal.use_external_phy)
      {
        gpio_set_drive_capability((gpio_num_t)USBPHY_DM_NUM, GPIO_DRIVE_CAP_3);
        gpio_set_drive_capability((gpio_num_t)USBPHY_DP_NUM, GPIO_DRIVE_CAP_3);
      }
    }
  }
}