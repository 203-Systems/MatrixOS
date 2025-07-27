#pragma once
#include "Point.h"
#include "Direction.h"

class Dimension {
 public:
  int16_t x, y;
  Dimension() {
    x = 0;
    y = 0;
  }

  Dimension(int16_t x, int16_t y) {
    this->x = x;
    this->y = y;
  }

  Dimension(uint32_t rawByte) {
    x = (int16_t)(rawByte >> 16);
    y = (int16_t)(rawByte & 0xFFFF);
  }

  bool Contains(Point point) { return point.x >= 0 && point.y >= 0 && point.x < x && point.y < y; }

  uint32_t Area() { return x * y; }

  Dimension operator+(const Dimension& cp) const  // cp stands for compare target
  {
    return Dimension(x + cp.x, y + cp.y);
  }

  bool operator!=(const Dimension& cp) const { return cp.x != x || cp.y != y; }

  bool operator<(const Dimension& cp) const { return x < cp.x || (x == cp.x && y < cp.y); }

  operator bool() { return x != 0 && y != 0; }

  operator uint32_t() { return (uint32_t)(x << 16 & y); }
};