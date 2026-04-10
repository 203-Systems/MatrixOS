#include "Device.h"
#include "MatrixOSConfig.h"
#include "WS2812/WS2812.h"

namespace Device
{
namespace LED
{
static vector<uint16_t> rotationMapping;

static uint16_t CanonicalXY2Index(Point xy) {
  if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8) // Main grid
  {
    uint16_t rowOffset = xy.y * 8;
    return rowOffset + ((xy.y % 2 == 0) ? xy.x : (7 - xy.x));
  }
  else if (xy.x == 8 && xy.y >= 0 && xy.y < 8) // Underglow Right Column
  {
    return 64 + (7 - xy.y);
  }
  else if (xy.y == 8 && xy.x >= 0 && xy.x < 8) // Underglow Bottom Row
  {
    return 88 + xy.x;
  }
  else if (xy.x == -1 && xy.y >= 0 && xy.y < 8) // Underglow Left Column
  {
    return 80 + xy.y;
  }
  else if (xy.y == -1 && xy.x >= 0 && xy.x < 8) // Underglow Top Row
  {
    return 72 + (7 - xy.x);
  }
  return UINT16_MAX;
}

static Point CanonicalIndex2XY(uint16_t index) {
  Point hw;
  if (index < 64)
  {
    int16_t y = index / 8;
    int16_t rowIndex = index % 8;
    hw = Point((y % 2 == 0) ? rowIndex : (7 - rowIndex), y);
  }
  else if (index < 72)
  {
    hw = Point(7 - (index - 64), 8);
  }
  else if (index < 80)
  {
    hw = Point(-1, index - 72);
  }
  else if (index < 88)
  {
    hw = Point(index - 80, -1);
  }
  else
  {
    return Point::Invalid();
  }
  return hw;
}

void UpdateRotationMapping(Direction rotation) {
  rotationMapping.resize(count);

  const Point dimension(Device::xSize, Device::ySize);
  for (uint16_t physicalIndex = 0; physicalIndex < count; physicalIndex++)
  {
    Point physicalPoint = CanonicalIndex2XY(physicalIndex);
    if (!physicalPoint)
    {
      rotationMapping[physicalIndex] = physicalIndex;
      continue;
    }

    Point sourcePoint = physicalPoint.Rotate(rotation, dimension, true);
    uint16_t sourceIndex = CanonicalXY2Index(sourcePoint);
    rotationMapping[physicalIndex] = (sourceIndex == UINT16_MAX) ? physicalIndex : sourceIndex;
  }
}

void Init() {
  WS2812::Init(ledPin, partitions);
  UpdateRotationMapping(TOP);
}

void Start() {}

IRAM_ATTR void Update(Color* frameBuffer, vector<uint8_t>& brightness) // Render LED
{
  const uint16_t* mapping = nullptr;
  if (!rotationMapping.empty() && Device::GetRotation() != TOP)
  {
    mapping = rotationMapping.data();
  }
  WS2812::Show(frameBuffer, brightness, mapping);
}

uint16_t XY2Index(Point xy) {
  return CanonicalXY2Index(xy);
}

Point Index2XY(uint16_t index) {
  return CanonicalIndex2XY(index);
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
  case 3:  // Underglow
    break; // TODO: Underglow
  }
  return UINT16_MAX;
}
} // namespace LED
} // namespace Device
