// Define Device Specific Function
#include "Device.h"

// Device Configs
#include "EVT1/Config.h"

void BurnEFuse();  // This is in device folder, a custom BurnEFuse will be provided

namespace Device
{
  void LoadVarientInfo() {
// Define device version to add factory functions
#ifdef FACTORY_CONFIG
#define DEViCE_XSTR(x) STR(x)
#define DEViCE_STR(x) #x
#pragma message("Factory config - Type 60 " DEViCE_XSTR(FACTORY_CONFIG))
    memcpy(&deviceInfo, &FACTORY_CONFIG::deviceInfo, sizeof(DeviceInfo));
    deviceInfo.ProductionYear = FACTORY_MFG_YEAR;
    deviceInfo.ProductionMonth = FACTORY_MFG_MONTH;
#endif

    ESP_LOGI("Device Init", "Loading config for Type 60 %.4s (MFG: %02d-%02d) (Serial: %s)", deviceInfo.Revision,
             deviceInfo.ProductionYear, deviceInfo.ProductionMonth, GetSerial().c_str());  // It seems excesive but
                                                                                           // deviceInfo.Revision does
                                                                                           // not have null terminator
    if (string(deviceInfo.Revision).compare(0, 4, "EVT1") == 0)
    { LoadEVT1(); }
    else
    {
      ESP_LOGE("Device Init", "Failed to find config for Type 60 %.4s", deviceInfo.Revision);
      LoadEVT1();
#ifdef FACTORY_CONFIG
      MatrixOS::SYS::ErrorHandler("Unable to find device config");
#endif
    }
  }
}