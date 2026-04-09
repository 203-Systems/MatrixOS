#include "MatrixOS.h"
#include "KeyPad.h"
#include <string>

namespace MatrixOS::KeyPad
{
KeyInfo* GetKey(Point keyXY) {
  return GetKey(XY2ID(keyXY));
}

KeyInfo* GetKey(uint16_t keyID) {
  return Device::KeyPad::GetKey(keyID);
}

void Clear() {
  Device::KeyPad::Clear();
}

uint16_t XY2ID(Point xy) // Delegates to device handlers for coordinate mapping (rotation handled by device layer)
{
  if (!xy)
    return UINT16_MAX;
  // Try all coordinate-capable clusters, preferring cluster function pointers
  for (const auto& cluster : Device::Input::clusters)
  {
    if (!cluster.HasCoordinates())
      continue;
    uint16_t memberId;
    bool found = cluster.tryGetMemberId
      ? cluster.tryGetMemberId(cluster, xy, &memberId)
      : Device::Input::TryGetMemberId(cluster.clusterId, xy, &memberId);
    if (found)
    {
      return Device::KeyPad::InputIdToLegacyKeyId(InputId{cluster.clusterId, memberId});
    }
  }
  return UINT16_MAX;
}

Point ID2XY(uint16_t keyID) // Delegates to device handlers for coordinate mapping (rotation handled by device layer)
{
  InputId id = Device::KeyPad::BridgeKeyId(keyID);
  // Prefer cluster function pointer, fall back to device-level free function
  const auto& clusters = Device::Input::clusters;
  for (const auto& cluster : clusters)
  {
    if (cluster.clusterId == id.clusterId && cluster.getPosition)
    {
      Point point;
      if (cluster.getPosition(cluster, id.memberId, &point))
      {
        return point;
      }
      return Point(INT16_MIN, INT16_MIN);
    }
  }
  Point point;
  if (Device::Input::GetPosition(id.clusterId, id.memberId, &point))
  {
    return point;
  }
  return Point(INT16_MIN, INT16_MIN);
}
} // namespace MatrixOS::KeyPad