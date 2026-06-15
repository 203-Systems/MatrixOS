#include "Device.h"
#include "MatrixOS.h"

#include "esp_private/usb_phy.h"

namespace Device
{
namespace USB
{

static usb_phy_handle_t phy_hdl = nullptr;
static bool serialJtagModeActive = false;

static void ReleaseOtgPhy() {
  if (phy_hdl == nullptr)
  {
    return;
  }

  esp_err_t result = usb_del_phy(phy_hdl);
  if (result == ESP_OK)
  {
    phy_hdl = nullptr;
  }
  else
  {
    MLOGE("Device USB", "Failed to release OTG PHY: %d", result);
  }
}

static bool EnableOtgPhy() {
  if (phy_hdl != nullptr)
  {
    return true;
  }

  usb_phy_config_t phy_conf = {
      .controller = USB_PHY_CTRL_OTG,
      .target = USB_PHY_TARGET_INT,
      .otg_mode = USB_OTG_MODE_DEVICE,
      .otg_speed = USB_PHY_SPEED_UNDEFINED,
  };

  esp_err_t result = usb_new_phy(&phy_conf, &phy_hdl);
  if (result != ESP_OK)
  {
    MLOGE("Device USB", "Failed to init OTG PHY: %d", result);
    phy_hdl = nullptr;
    return false;
  }
  return true;
}

void Init() {
#ifdef MATRIXOS_BUILD_INDEV
  if (serialJtagUsbMode.Get())
  {
    ReleaseOtgPhy();
    serialJtagModeActive = true;
    return;
  }
#endif

  serialJtagModeActive = false;
  EnableOtgPhy();
}

bool SetSerialJtagMode(bool enabled, bool save) {
#ifdef MATRIXOS_BUILD_INDEV
  if (save)
  {
    serialJtagUsbMode.Set(enabled);
  }

  return true;
#else
  (void)save;
  return !enabled;
#endif
}

bool SerialJtagModeEnabled() {
#ifdef MATRIXOS_BUILD_INDEV
  return serialJtagModeActive || serialJtagUsbMode.Get();
#else
  return false;
#endif
}
} // namespace USB
} // namespace Device
