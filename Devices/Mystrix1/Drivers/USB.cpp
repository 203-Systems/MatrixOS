#include "Device.h"

#include "esp_private/usb_phy.h"

namespace Device
{
  namespace USB
  {

    static usb_phy_handle_t phy_hdl;

    void Init() {
      usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
        .target = USB_PHY_TARGET_INT,
        .otg_mode = USB_OTG_MODE_DEVICE,
        .otg_speed = USB_PHY_SPEED_UNDEFINED,
      };

      usb_new_phy(&phy_conf, &phy_hdl);
    }
  }
}