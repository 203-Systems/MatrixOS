// Define Device Specific Function
#include "Device.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

// Device Configs
#include "V100/Config.h"
#include "V110/Config.h"
#include "RevC/Config.h"

void BurnEFuse();  // This is in device folder, a custom BurnEFuse will be provided

namespace Device
{
  void LoadVariantInfo() 
  {
    // Load Factory Config
    #ifdef FACTORY_CONFIG
        #pragma message "FACTORY_CONFIG IS USED"
        DeviceInfo factoryDeviceInfo{FACTORY_DEVICE_MODEL, FACTORY_DEVICE_REVISION, FACTORY_MFG_YEAR, FACTORY_MFG_MONTH};
        memcpy (&device_info, &factoryDeviceInfo, sizeof(DeviceInfo));
        ESP_LOGI("Device Init", "Factory config - Mystrix %.4s %.4s", device_info.model, device_info.revision);
    #endif

    // Load Velocity Sensitive Config
    if(device_info.model[3] == 'P')
    {
      name += " Pro";
      product_name += " Pro";
      model = "MX1P";
      KeyPad::velocity_sensitivity = true;
    }
    else if(device_info.model[3] == 'S')
    {
      KeyPad::velocity_sensitivity = false;
      // Remove "Underglow" from partitions
      LED::partitions.pop_back();
    }
    else
    { 
      // Default to Mystrix Pro because RC units has no config baked in
      name += " Pro";
      product_name += " Pro";
      model = "MX1P";
      KeyPad::velocity_sensitivity = true;
      ESP_LOGE("Device Init", "Failed to find config for %.4s %.4s", device_info.model, device_info.revision);
    }

    ESP_LOGI("Device Init", "Loading config for %s (%.4s %.4s) (MFG: %02d-%02d) (Serial: %s)", Device::name.c_str(), device_info.model, device_info.revision,
             device_info.production_year, device_info.production_month, GetSerial().c_str());  // It seems excessive but
                                                                                           // device_info.revision does
                                                                                           // not have null terminator
    if (string(device_info.revision).compare(0, 4, "V100") == 0)
    { LoadV100(); }
    else if (string(device_info.revision).compare(0, 4, "V110") == 0)
    { LoadV110(); }
    else if (string(device_info.revision).compare(0, 4, "REVC") == 0)
    { LoadRevC(); }
    else
    {
    ESP_LOGE("Device Init", "Failed to find config for %s %.4s", Device::name.c_str(), device_info.revision);
      LoadV110();
    }
  }
}
