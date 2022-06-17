//Define Device Specific Function
#include "Device.h"
#include "V100/Config.h"

void BurnEFuse(); //This is in device folder, a custom BurnEFuse will be provided

namespace Device
{
    void LoadVarientInfo()
    {
        //Define device version to add factory functions
        #ifdef FACTORY_CONFIG
        //Todo Add V1.1
        #pragma message("Factory config - Matrix Pro " FACTORY_CONFIG)
        memcpy(&deviceInfo, &FACTORY_CONFIG::deviceInfo, sizeof(DeviceInfo));
        deviceInfo.ProductionYear = FACTORY_MFG_YEAR;
        deviceInfo.ProductionMonth = FACTORY_MFG_MONTH;
        #endif

        ESP_LOGI("Device Init", "Loading config for Matrix Pro %.4s (MFG: %02d-%02d)", deviceInfo.Revision, deviceInfo.ProductionYear, deviceInfo.ProductionMonth); //It seems excesive but deviceInfo.Revision does not have null terminator
        if(string(deviceInfo.Revision).compare("V100"))
        {
            LoadV100();
        }
        else
        {
            ESP_LOGE("Device Init", "Failed to find config for Matrix Pro %.4s", deviceInfo.Revision);
            LoadV100();
        }
    }
}