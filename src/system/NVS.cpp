#include "MatrixOS.h"

namespace MatrixOS::NVS
{
    vector<char> GetVariable(string name)
    {
        return Device::NVS::Read(name);
    }

    bool SetVariable(string name, void* pointer, uint16_t length)
    {   
        ESP_LOGI("NVS", "Variable wrote : %s : %d: %d", name.c_str(), *(uint32_t*)pointer, length);

        return Device::NVS::Write(name, pointer, length);
    }

    bool DeleteVariable(string name)
    {
        return Device::NVS::Delete(name);
    }
}