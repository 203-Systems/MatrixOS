#include "MatrixOS.h"

enum ActionType: uint8_t
{
  Action,
  Effect
};

enum ActionIndexType: uint8_t
{
  ID,
  COORD,
};

struct ActionInfo
{
  ActionType actionType;
  ActionIndexType indexType;
  union
  {
    uint32_t ID;
    Point coord = Point::Invalid();
  };
  uint8_t layer;
  uint8_t index;
};
