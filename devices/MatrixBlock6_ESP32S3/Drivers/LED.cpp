#include "Device.h"

namespace Device
{
  namespace LED
  {
    void Init() {
      WS2812::Init(RMT_CHANNEL_0, led_pin, led_chunk_count, led_chunk);
    }

    void Start() {}

    void Update(Color* frameBuffer, uint8_t brightness)  // Render LED
    {
      // ESP_LOGI("LED", "LED Update");
      // ESP_LOGI("LED", "%d", frameBuffer[0].RGB());
      WS2812::Show(frameBuffer, brightness);
    }

    uint16_t XY2Index(Point xy) {
      if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)  // Main grid
      { return xy.x + xy.y * 8; }
      else if (xy.x == 8 && xy.y >= 0 && xy.y < 8)  // Underglow Right Column
      { return 64 + (7 - xy.y); }
      else if (xy.y == 8 && xy.x >= 0 && xy.x < 8)  // Underglow Bottom Row
      { return 88 + xy.x; }
      else if (xy.x == -1 && xy.y >= 0 && xy.y < 8)  // Underglow Left Column
      { return 80 + xy.y; }
      else if (xy.y == -1 && xy.x >= 0 && xy.x < 8)  // Underglow Top Row
      { return 72 + (7 - xy.x); }
      return UINT16_MAX;
    }

    // Point Index2XY(uint16_t index)
    // {
    //     if(xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8) //Main grid
    //     {
    //         return xy.x + xy.y * 8;
    //     }
    //     if(index < 64)
    //     {

    //     }
    //     return UINT16_MAX;
    // }

    // TODO This text is very wrong (GRID)
    // Matrix use the following ID Struct
    //  CCCC IIIIIIIIIIII
    //  C as class (4 bits), I as index (12 bits). I could be splited by the class defination, for example, class 1
    //  (grid), it's splited to XXXXXXX YYYYYYY. Class List: Class 0 - Raw Index - IIIIIIIIIIII Class 1 - Grid - XXXXXX
    //  YYYYYY Class 2 - TouchBar - IIIIIIIIIIII Class 3 - Underglow - IIIIIIIIIIII

    uint16_t ID2Index(uint16_t ledID) {
      uint8_t ledClass = ledID >> 12;
      switch (ledClass)
      {
        case 0:
          if (ledID < numsOfLED)
            return ledID;
          break;
        case 3:   // Underglow
          break;  // TODO: Underglow
      }
      return UINT16_MAX;
    }
  }
}