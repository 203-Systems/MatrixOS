#pragma once

#define FRACT16_MAX UINT16_MAX
class Fract16 {
 public:
  uint16_t value;

  Fract16(uint16_t value = 0) { this->value = value; }

  Fract16(uint16_t value, uint8_t bits) {
    this->value = value << (16 - bits);
  }

  // uint8_t to14bits(){return value >> 2;}
  // uint8_t to12bits(){return value >> 4;}
  // uint8_t to10bits(){return value >> 6;}
  uint8_t to8bits() { return value >> 8; }
  uint8_t to7bits() { return value >> 9; }

  operator bool() { return value > 0; }
  operator uint8_t() { return to8bits(); }
  operator uint16_t() { return value; }
  operator uint32_t() { return value; }
  operator float() { return (float)value / UINT16_MAX; }
  operator int() { return value; }

  bool operator<(int value) { return this->value < value; }
  bool operator<(Fract16 value) { return this->value < (uint16_t)value; }

  bool operator<=(int value) { return this->value <= value; }
  bool operator<=(Fract16 value) { return this->value <= (uint16_t)value; }

  bool operator>(int value) { return this->value > value; }
  bool operator>(Fract16 value) { return this->value > (uint16_t)value; }

  bool operator>=(int value) { return this->value >= value; }
  bool operator>=(Fract16 value) { return this->value >= (uint16_t)value; }

  bool operator==(int value) { return this->value == value; }
  bool operator==(Fract16 value) { return this->value == (uint16_t)value; }

  bool operator!=(int value) { return this->value != value; }
  bool operator!=(Fract16 value) { return this->value != (uint16_t)value; }

  Fract16 operator+(Fract16 value) { 
    if((uint16_t)value + this->value > FRACT16_MAX) {return FRACT16_MAX;}
    return this->value + (uint16_t)value;
  }

  Fract16 operator-(Fract16 value) { 
    if(value >= this->value) {return 0;}
    return this->value - (uint16_t)value;
  }
};