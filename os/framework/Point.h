#pragma once
#include "Direction.h"

class Point {
 public:
  int16_t x, y;

  Point(int16_t x, int16_t y) {
    this->x = x;
    this->y = y;
  }

  Point(uint32_t rawByte) {
    x = (int16_t)(rawByte >> 16);
    y = (int16_t)(rawByte & 0xFFFF);
  }

  Point operator+(const Point& cp) const  // cp stands for compare target
  {
    return Point(x + cp.x, y + cp.y);
  }

  Point operator-(const Point& cp) const  // cp stands for compare target
  {
    return Point(x - cp.x, y - cp.y);
  }

  bool operator==(const Point& cp) const { return cp.x == x && cp.y == y; }

  bool operator!=(const Point& cp) const { return cp.x != x || cp.y != y; }

  bool operator<(const Point& cp) const { return x < cp.x || (x == cp.x && y < cp.y); }

  Point operator*(const int val) const { return Point(x * val, y * val); }

  Point operator/(const int val) const { return Point(x / val, y / val); }

  operator bool() { return x != INT16_MIN && y != INT16_MIN; }

  operator uint32_t() { return (uint32_t)(x << 16 & y); }

  static Point Invalid() { return Point(INT16_MIN, INT16_MIN); }

  Point Rotate(EDirection rotation, Point dimension, bool reverse = false) {
    int16_t new_x;
    int16_t new_y;
    // if(bool() == false)
    // 	return *this;
    if (reverse)
      rotation = (EDirection)(360 - rotation);
    switch (rotation)
    {
      case RIGHT:
        new_x = (dimension.x - 1) - y;
        new_y = x;
        break;
      case DOWN:
        new_x = (dimension.x - 1) - x;
        new_y = (dimension.y - 1) - y;
        break;
      case LEFT:
        new_x = y;
        new_y = (dimension.y - 1) - x;
        break;
      default:
        new_x = x;
        new_y = y;
    }
    return Point(new_x, new_y);
  }
};