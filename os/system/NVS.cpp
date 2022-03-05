#include "MatrixOS.h"

namespace MatrixOS::NVS
{
    vector<char> GetVariable(string name)
    {
        return Device::NVS::Read(name);
    }

    int8_t GetVariable(string name, void* pointer, uint16_t length)
    {
        vector<char> data = Device::NVS::Read(name);
        if(data.size() == 0) //Havn't been saved
        {
            SetVariable(name, pointer, length);
            return 1;
        }
        else if(data.size() != length) //Size missmatched
        {
            return 2;
        }
        else if(data.size() == length) //Size matched
        {
            memcpy(pointer, data.data(), length);
            return 0;
        }        
        return -1;
    }

    bool SetVariable(string name, void* pointer, uint16_t length)
    {   
        MatrixOS::Logging::LogVerbose("NVS", "Variable wrote : %s : %d: %d", name.c_str(), *(uint32_t*)pointer, length);
        return Device::NVS::Write(name, pointer, length);
    }

    bool DeleteVariable(string name)
    {
        return Device::NVS::Delete(name);
    }
}