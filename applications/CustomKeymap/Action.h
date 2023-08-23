#include "MatrixOS.h"

enum ActionType: uint8_t
{
  ACTION,
  EFFECT
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
    uint32_t id;
    Point coord = Point::Invalid();
  };
  uint8_t layer;
  uint8_t index;
  uint8_t depth = 0; // Prevent recursive calls

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
