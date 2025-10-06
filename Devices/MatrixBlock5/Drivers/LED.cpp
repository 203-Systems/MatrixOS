#include "Device.h"
#include "MatrixOSConfig.h"
#include "WS2812/WS2812.h"

// TIM8 handle for WS2812 LED driver
extern TIM_HandleTypeDef htim8;

// Forward declarations
extern void MX_DMA_Init(void);
extern void MX_TIM8_Init(void);

namespace Device
{
  namespace LED
  {
    void Init() {
      MX_DMA_Init();
      MX_TIM8_Init();
      WS2812::Init(&htim8, TIM_CHANNEL_2, Device::LED::count);
    }

    void Start() {
      // LED startup tasks - none needed for MatrixBlock5
    }

    void Update(Color* frameBuffer, vector<uint8_t>& brightness)  // Render LED
    {
      WS2812::Show(frameBuffer, brightness);
    }

    uint16_t XY2Index(Point xy) {
      if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)  // Main grid
      { return xy.x + xy.y * 8; }
      return UINT16_MAX;
    }

    Point Index2XY(uint16_t index)
    {
      if (index < 64)
      {
        return Point(index % 8, index / 8);
      }
      return Point::Invalid();
    }

    // Matrix use the following ID Struct
    //  CCCC IIIIIIIIIIII
    //  C as class (4 bits), I as index (12 bits). I could be split by the class definition, for example, class 1
    //  (grid), it's split to XXXXXXX YYYYYYY. Class List: Class 0 - Raw Index - IIIIIIIIIIII Class 1 - Grid - XXXXXX
    //  YYYYYY Class 2 - TouchBar - IIIIIIIIIIII Class 3 - Underglow - IIIIIIIIIIII

    uint16_t ID2Index(uint16_t ledID) {
      uint8_t ledClass = ledID >> 12;
      switch (ledClass)
      {
        case 0:
          if (ledID < LED::count)
            return ledID;
          break;
        case 1:  // Main Grid
        {
          uint16_t index = ledID & (0b0000111111111111);
          if (index < 64)
            return index;
          break;
        }
        case 3:   // Underglow
          break;  // TODO: Underglow
      }
      return UINT16_MAX;
    }
  }
}