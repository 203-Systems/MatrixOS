// Define Device Specific Function
#include "Device.h"

// Device Configs
#include "V100/Config.h"
#include "V110/Config.h"
#include "RevC/Config.h"

void BurnEFuse();  // This is in device folder, a custom BurnEFuse will be provided

namespace Device
{
  void LoadVarientInfo() {
// Define device version to add factory functions
#ifdef FACTORY_CONFIG
    #define DEViCE_XSTR(x) STR(x)
    #define DEViCE_STR(x) #x
    #pragma message("Factory config - Matrix" DEViCE_XSTR(FACTORY_DEVICE_VERSION) DEViCE_XSTR(FACTORY_CONFIG))
    memcpy(&deviceInfo, &FACTORY_CONFIG::deviceInfo, sizeof(DeviceInfo));
#endif

    // Load Velocity Sensitive Config
    if(deviceInfo.DeviceCode[3] == 'P')
    {
      KeyPad::keypad_config.velocity_sensitive = true;
      Device::name += " Pro";
      Device::model = "MX1P";

    }
    else if(deviceInfo.DeviceCode[3] == 'S')
    {
      KeyPad::keypad_config.velocity_sensitive = false;
    }
    else
    {
      KeyPad::keypad_config.velocity_sensitive = false;
    }

    ESP_LOGI("Device Init", "Loading config for %s %.4s (MFG: %02d-%02d) (Serial: %s)", Device::name.c_str(), deviceInfo.Revision,
             deviceInfo.ProductionYear, deviceInfo.ProductionMonth, GetSerial().c_str());  // It seems excesive but
                                                                                           // deviceInfo.Revision does
                                                                                           // not have null terminator
    if (string(deviceInfo.Revision).compare(0, 4, "V100") == 0)
    { LoadV100(); }
    else if (string(deviceInfo.Revision).compare(0, 4, "V110") == 0)
    { LoadV110(); }
    else if (string(deviceInfo.Revision).compare(0, 4, "REVC") == 0)
    { LoadRevC(); }
    else
    {
      ESP_LOGE("Device Init", "Failed to find config for %s %.4s", Device::name.c_str(), deviceInfo.Revision);
      LoadV110();
    }
  }
}