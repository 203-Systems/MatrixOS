// Define Device Specific Function
#include "Device.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

// Device Configs
#include "V100/Config.h"
#include "V110/Config.h"
#include "RevC/Config.h"

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
  if (deviceInfo.model[3] == 'P')
  {
    name += " Pro";
    productName += " Pro";
    model = "MX1P";
    KeyPad::velocitySensitivity = true;
  }
  else if (deviceInfo.model[3] == 'S')
  {
    KeyPad::velocitySensitivity = false;
    // Remove "Underglow" from partitions
    LED::partitions.pop_back();
  }
  else
  {
    // Default to Mystrix Pro because RC units has no config baked in
    name += " Pro";
    productName += " Pro";
    model = "MX1P";
    KeyPad::velocitySensitivity = true;
    ESP_LOGE("Device Init", "Failed to find config for %.4s %.4s", deviceInfo.model, deviceInfo.revision);
  }

  ESP_LOGI("Device Init", "Loading config for %s (%.4s %.4s) (MFG: %02d-%02d) (Serial: %s)", Device::name.c_str(), deviceInfo.model,
           deviceInfo.revision, deviceInfo.productionYear, deviceInfo.productionMonth,
           GetSerial().c_str()); // It seems excessive but
                                 // deviceInfo.revision does
                                 // not have null terminator
  if (string(deviceInfo.revision).compare(0, 4, "V100") == 0)
  {
    LoadV100();
  }
  else if (string(deviceInfo.revision).compare(0, 4, "V110") == 0)
  {
    LoadV110();
  }
  else if (string(deviceInfo.revision).compare(0, 4, "REVC") == 0)
  {
    LoadRevC();
  }
  else
  {
    ESP_LOGE("Device Init", "Failed to find config for %s %.4s", Device::name.c_str(), deviceInfo.revision);
    LoadV110();
  }
}
} // namespace Device
