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

  bool operator==(const ActionInfo& other) const
  {
    return memcmp(this, &other, sizeof(ActionInfo)) == 0;
  }
};

struct ActionInfoHash
{
  size_t operator()(ActionInfo const& actionInfo) const
  {
    return FNV1aHash((const char*)&actionInfo, sizeof(ActionInfo));
  }
};
