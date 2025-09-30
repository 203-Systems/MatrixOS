#include "Device.h"
#include "MatrixOSConfig.h"
#include "WS2812/WS2812.h"

namespace Device
{
  namespace LED
  {
    void Init() {
      WS2812::Init(led_pin, partitions);
    }

    void Start() {}

    IRAM_ATTR void Update(Color* frameBuffer, vector<uint8_t>& brightness)  // Render LED
    {
      WS2812::Show(frameBuffer, brightness);
    }

    uint16_t XY2Index(Point xy) {
      if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)  // Main grid
      { return xy.x + xy.y * 8; }
      else if(underglow)
      {
        if (xy.x == 8 && xy.y >= 0 && xy.y < 8)  // Underglow Right Column
        { return 64 + (7 - xy.y); }
        else if (xy.y == 8 && xy.x >= 0 && xy.x < 8)  // Underglow Bottom Row
        { return 88 + xy.x; }
        else if (xy.x == -1 && xy.y >= 0 && xy.y < 8)  // Underglow Left Column
        { return 80 + xy.y; }
        else if (xy.y == -1 && xy.x >= 0 && xy.x < 8)  // Underglow Top Row
        { return 72 + (7 - xy.x); }
      }
      return UINT16_MAX;
    }

    Point Index2XY(uint16_t index)
    {
      if (index < 64)
      {
        return Point(index % 8, index / 8);
      }
      else if (index < 72)
      {
        return Point(7 - (index - 64), 8);
      }
      else if (index < 80)
      {
        return Point(-1, index - 72);
      }
      else if (index < 88)
      {
        return Point(index - 80, -1);
      }
      return Point::Invalid();
    }

    // TODO This text is very wrong (GRID)
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
        case 3:   // Underglow
          break;  // TODO: Underglow
      }
      return UINT16_MAX;
    }
  }
}