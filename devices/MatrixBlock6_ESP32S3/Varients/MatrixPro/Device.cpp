//Define Device Specific Function
#include "Device.h"

//Device Configs
#include "V100/Config.h"
#include "V110/Config.h"

void BurnEFuse(); //This is in device folder, a custom BurnEFuse will be provided

namespace Device
{
    void LoadVarientInfo()
    {
        //Define device version to add factory functions
        #ifdef FACTORY_CONFIG
        #define DEViCE_XSTR(x) STR(x)
        #define DEViCE_STR(x) #x
        #pragma message("Factory config - Matrix Pro " DEViCE_XSTR(FACTORY_CONFIG))
        memcpy(&deviceInfo, &FACTORY_CONFIG::deviceInfo, sizeof(DeviceInfo));
        deviceInfo.ProductionYear = FACTORY_MFG_YEAR;
        deviceInfo.ProductionMonth = FACTORY_MFG_MONTH;
        #endif

        ESP_LOGI("Device Init", "Loading config for Matrix Pro %.4s (MFG: %02d-%02d) (Serial: %s)", deviceInfo.Revision, deviceInfo.ProductionYear, deviceInfo.ProductionMonth, GetSerial().c_str()); //It seems excesive but deviceInfo.Revision does not have null terminator
        if(string(deviceInfo.Revision).compare(0, 4, "V100") == 0)
        {
            LoadV100();
        }
        else if(string(deviceInfo.Revision).compare(0, 4, "V110") == 0)
        {
            LoadV110();
        }
        else
        {
            ESP_LOGE("Device Init", "Failed to find config for Matrix Pro %.4s", deviceInfo.Revision);
            LoadV100();
            #ifdef FACTORY_CONFIG
            MatrixOS::SYS::ErrorHandler("Unable to find device config");
            #endif
        }
    }
}