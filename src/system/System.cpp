#include "MatrixOS.h"

namespace MatrixOS
{
  namespace SYS
  {
    uintptr_t GetAttribute(EAttribute attribute)
    {
      switch (attribute)
      {
        //System Attributes
        case EAttribute::SystemVersion:


        //Hardware Attributes
        case EAttribute::DeviceType: 
        case EAttribute::DeviceVersion: 
        case EAttribute::DerviceRevision: 
        case EAttribute::DeviceBatch: 
        case EAttribute::SerialNumber:
        case EAttribute::VelocityRange:
        case EAttribute::LEDType: 
        case EAttribute::LEDSizeX: 
        case EAttribute::LEDSizeY: 
        case EAttribute::BootloaderVersion: 

        default:
          _ASSERT(0);
          return (uintptr_t)nullptr;
      }
    }
  }
}