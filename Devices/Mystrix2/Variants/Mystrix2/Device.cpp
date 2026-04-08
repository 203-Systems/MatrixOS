// Define Device Specific Function
#include "Device.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

// Device Configs
#include "UltraProto2/Config.h"

void BurnEFuse(); // This is in device folder, a custom BurnEFuse will be provided

namespace Device
{
void LoadVariantInfo() {
// Load Factory Config
#ifdef FACTORY_CONFIG
#pragma message "FACTORY_CONFIG IS USED"
  DeviceInfo factoryDeviceInfo{FACTORY_DEVICE_MODEL, FACTORY_DEVICE_REVISION, FACTORY_MFG_YEAR, FACTORY_MFG_MONTH};
  memcpy(&deviceInfo, &factoryDeviceInfo, sizeof(DeviceInfo));
  ESP_LOGI("Device Init", "Factory config - Mystrix %.4s %.4s", deviceInfo.model, deviceInfo.revision);
#endif

  // Load Velocity Sensitive Config
  if (memcmp(deviceInfo.model, "MX2U", 4) == 0)
  {
    name += " Ultra";
    productName += " Ultra";
    model = "MX2U";
    KeyPad::velocitySensitivity = true;
  }
  else
  {
    // Default to Mystrix 2 Ultra if the model code is missing or unknown.
    name += " Ultra";
    productName += " Ultra";
    model = "MX2U";
    KeyPad::velocitySensitivity = true;
    ESP_LOGE("Device Init", "Failed to find device config. Default to Mystrix 2 Ultra");
  }

  ESP_LOGI("Device Init", "Loading config for %s (%.4s %.4s) (MFG: %02d-%02d) (Serial: %s)", Device::name.c_str(), deviceInfo.model,
           deviceInfo.revision, deviceInfo.productionYear, deviceInfo.productionMonth,
           GetSerial().c_str()); // It seems excessive but
                                 // deviceInfo.revision does
                                 // not have null terminator
  if (string(deviceInfo.revision).compare(0, 4, "MX2U") == 0)
  {
    LoadUPT2();
  }
  else
  {
    ESP_LOGE("Device Init", "Failed to find config for %s %.4s, defaulting to MX2U", Device::name.c_str(), deviceInfo.revision);
    LoadUPT2();
  }
}
} // namespace Device
