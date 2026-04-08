#pragma once
#include "Direction.h"

class Point {
public:
  int16_t x, y;

  Point() {
    x = y = INT16_MIN;
  }

  Point(int16_t x, int16_t y) {
    this->x = x;
    this->y = y;
  }

  Point(uint32_t rawByte) {
    x = (int16_t)(rawByte >> 16);
    y = (int16_t)(rawByte & 0xFFFF);
  }

  Point operator+(const Point& cp) const // cp stands for compare target
  {
    return Point(x + cp.x, y + cp.y);
  }

  Point operator-(const Point& cp) const // cp stands for compare target
  {
    return Point(x - cp.x, y - cp.y);
  }

  bool operator==(const Point& cp) const {
    return cp.x == x && cp.y == y;
  }

  bool operator!=(const Point& cp) const {
    return cp.x != x || cp.y != y;
  }

  bool operator<(const Point& cp) const {
    return x < cp.x || (x == cp.x && y < cp.y);
  }

  Point operator*(const int val) const {
    return Point(x * val, y * val);
  }

  Point operator/(const int val) const {
    return Point(x / val, y / val);
  }

  operator bool() {
    return x != INT16_MIN && y != INT16_MIN;
  }

  operator uint32_t() {
    return (uint32_t)(x << 16 & y);
  }

  static Point Origin() {
    return Point(0, 0);
  }
  static Point Invalid() {
    return Point(INT16_MIN, INT16_MIN);
  }

  Point Rotate(Direction rotation, Point dimension, bool reverse = false) {
    int16_t newX;
    int16_t newY;
    // if(bool() == false)
    // 	return *this;
    if (reverse)
      rotation = (Direction)(360 - rotation);
    switch (rotation)
    {
    case RIGHT:
      newX = (dimension.x - 1) - y;
      newY = x;
      break;
    case DOWN:
      newX = (dimension.x - 1) - x;
      newY = (dimension.y - 1) - y;
      break;
    case LEFT:
      newX = y;
      newY = (dimension.y - 1) - x;
      break;
    default:
      newX = x;
      newY = y;
    }
    return Point(newX, newY);
  }
};