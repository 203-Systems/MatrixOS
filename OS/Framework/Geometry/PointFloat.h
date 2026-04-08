#pragma once

class PointFloat {
public:
  float x;
  float y;

  PointFloat() : x(0), y(0) {}

  PointFloat(float x, float y) {
    this->x = x;
    this->y = y;
  }
};
