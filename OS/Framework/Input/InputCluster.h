#pragma once

#include "Types.h"
#include "InputId.h"
#include "InputClass.h"
#include "Point.h"
#include "Dimension.h"
#include "Direction.h"

struct InputCluster {
  uint8_t clusterId;
  string name;
  InputClass inputClass;
  InputClusterShape shape;
  Point rootPoint;
  Dimension dimension;
  uint16_t inputCount;

  // Device-specified rotation for this cluster's coordinates.
  // The device layer sets this based on the current device orientation.
  Direction rotation = TOP;
  // The bounding box used for the rotation transform (typically the main grid size).
  // Only used when rotation != TOP.
  Dimension rotationDimension = Dimension(0, 0);

  // Device-owned discrete mapping handlers.
  // Only meaningful for discrete coordinate-addressable clusters (Grid2D, Linear1D).
  // nullptr for Scalar or Area2D clusters.
  bool (*getPosition)(const InputCluster& cluster, uint16_t memberId, Point* point) = nullptr;
  bool (*tryGetMemberId)(const InputCluster& cluster, Point point, uint16_t* memberId) = nullptr;

  bool HasRootPoint() const {
    return rootPoint.x != INT16_MIN && rootPoint.y != INT16_MIN;
  }

  bool HasCoordinates() const {
    return HasRootPoint() && (shape == InputClusterShape::Linear1D || shape == InputClusterShape::Grid2D);
  }

  bool Contains(Point globalPoint) const {
    if (!HasCoordinates())
    {
      return false;
    }

    Point localPoint = globalPoint - rootPoint;
    return localPoint.x >= 0 && localPoint.y >= 0 && localPoint.x < dimension.x && localPoint.y < dimension.y;
  }
};
