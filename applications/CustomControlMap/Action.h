#include "MatrixOS.h"

enum ActionType: uint8_t
{
  ACTION,
  EFFECT,
};

enum ActionIndexType: uint8_t
{
  ID,
  COORD,
};

struct ActionInfo
{
  ActionType actionType:2;
  ActionIndexType indexType:2;
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

  // ActionType 2 bits, IndexType 2 bits, 
  // ID 14 bits or Coord 14 bits
  // Layer 6 bits, Index 8 bits
  size_t Hash() {
      size_t hash = 0;

      // Combine actionType and indexType
      hash |= (static_cast<size_t>(actionType & 0b11) << 30);
      hash |= (static_cast<size_t>(indexType & 0b11) << 28);

      // Depending on indexType, use ID or coordinates
      if (indexType == ActionIndexType::ID) {
          hash |= (static_cast<size_t>(id & 0x3FFF) << 14);
      } else {
          hash |= (static_cast<size_t>(coord.x & 0x7F) << 21);
          hash |= (static_cast<size_t>(coord.y & 0x7F) << 14);
      }

      // Add layer and index
      hash |= (static_cast<size_t>(layer & 0x3F) << 8);
      hash |= static_cast<size_t>(index & 0xFF);

      return hash;
  }
};