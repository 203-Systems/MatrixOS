#include "UAD.h"

bool UADRuntime::SetRegister(ActionInfo* actionInfo, uint32_t value)
{
    return registers.insert_or_assign(actionInfo->Hash(), value).second;
}

bool UADRuntime::GetRegister(ActionInfo* actionInfo, uint32_t* value)
{   
    std::map<uint32_t, uint32_t>::iterator it = registers.find(actionInfo->Hash());

    if(it == registers.end())
    {
        SetRegister(actionInfo, 0);
        *value = 0;
        return false;
    }

    *value = it->second;
    return true;
}

bool UADRuntime::ClearRegister(ActionInfo* actionInfo)
{
    return registers.erase(actionInfo->Hash()) > 0;
}

  void UADRuntime::SetLayerState(uint8_t layer, LayerInfoType type, bool state)
  {
    if(type == LayerInfoType::ACTIVE)
    {
        uint8_t oldTopLayer = GetTopLayer();
        if(state)
        {
            layerEnabled |= (1 << layer);
        }
        else
        {
            layerEnabled &= ~(1 << layer);
        }
        uint8_t newTopLayer = GetTopLayer();

        if(oldTopLayer != newTopLayer)
        {
            MatrixOS::LED::PauseUpdate(true);
            DeinitializeLayer(oldTopLayer);
            MatrixOS::LED::Fill(Color(0), 0);
            InitializeLayer(newTopLayer);
            MatrixOS::LED::PauseUpdate(false);
        }
    }
    else if(type == LayerInfoType::PASSTHROUGH)
    {
        if(state)
        {
            layerPassthrough |= (1 << layer);
        }
        else
        {
            layerPassthrough &= ~(1 << layer);
        }
    }
  }

bool UADRuntime::GetLayerState(uint8_t layer, LayerInfoType type)
{
    if(type == LayerInfoType::ACTIVE)
    {
        return IsBitSet(layerEnabled, layer);
    }
    else if(type == LayerInfoType::PASSTHROUGH)
    {
        return IsBitSet(layerPassthrough, layer);
    }
    return false;
}