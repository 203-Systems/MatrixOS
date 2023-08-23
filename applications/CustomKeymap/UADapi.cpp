#include "UAD.h"

bool UAD::SetRegister(ActionInfo* actionInfo, uint32_t value)
{
    return registers.insert_or_assign(*actionInfo, value).second;
}

bool UAD::GetRegister(ActionInfo* actionInfo, uint32_t* value)
{   
    std::unordered_map<ActionInfo, uint32_t>::iterator it = registers.find(*actionInfo);

    if(it == registers.end())
    {
        return false;
    }

    *value = it->second;
    return true;
}

bool UAD::ClearRegister(ActionInfo* actionInfo)
{
    return registers.erase(*actionInfo) > 0;
}

  void UAD::SetLayerState(uint8_t layer, LayerInfoType type, bool state)
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
            MatrixOS::LED::Fill(Color(0), 0);
            DeinitializeLayer(oldTopLayer);
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

bool UAD::GetLayerState(uint8_t layer, LayerInfoType type)
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

uint8_t UAD::GetLayerCount()
{
    return layerCount;
}