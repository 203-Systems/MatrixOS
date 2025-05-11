#include "Device.h"

#include "esp_private/periph_ctrl.h"
#include "esp_private/usb_phy.h"
#include "soc/usb_pins.h"

namespace Device
{
  namespace USB
  {

    static usb_phy_handle_t phy_hdl;

    void Init() {
        // Configure USB PHY
      usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
        .target = USB_PHY_TARGET_INT,
        .otg_mode = USB_OTG_MODE_DEVICE,
      };

      // OTG IOs config
      // const usb_phy_otg_io_conf_t otg_io_conf = USB_PHY_SELF_POWERED_DEVICE(config->vbus_monitor_io);
      // if (config->self_powered) {
      //   phy_conf.otg_io_conf = &otg_io_conf;
      // }
      // ESP_RETURN_ON_ERROR(usb_new_phy(&phy_conf, &phy_hdl), TAG, "Install USB PHY failed");

      usb_new_phy(&phy_conf, &phy_hdl);
    }
  }
}