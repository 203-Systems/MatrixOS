//Define Device Specific Function
#include "Device.h"
#include "V100/Config.h"

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
        deviceInfo.ProductionYear = FACTORY_MFG_MONTH;
        #endif

        ESP_LOGI("Device Init", "Loading config for Matrix Pro %s", string(deviceInfo.Revision).c_str()); //It seems excesive but deviceInfo.Revision does not have null terminator
        if(string(deviceInfo.Revision).compare("V100"))
        {
            LoadV100();
        }
        else
        {
            ESP_LOGE("Device Init", "Failed to find config for Matrix Pro %s", string(deviceInfo.Revision).c_str());
            LoadV100();
        }
    }
}