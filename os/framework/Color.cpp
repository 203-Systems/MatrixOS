#include "Color.h"
#include <cmath>

float fract(float x) { return x - int(x); }

float mix(float a, float b, float t) { return a + (b - a) * t; }

float step(float e, float x) { return x < e ? 0.0 : 1.0; }

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

Color::Color() {
  W = 0;
  R = 0;
  G = 0;
  B = 0;
}

Color::Color(uint32_t WRGB) {
  W = (WRGB & 0xFF000000) >> 24;
  R = (WRGB & 0x00FF0000) >> 16;
  G = (WRGB & 0x0000FF00) >> 8;
  B = (WRGB & 0x000000FF);
}

Color::Color(uint8_t nR, uint8_t nG, uint8_t nB, uint8_t nW) {
  R = nR;
  G = nG;
  B = nB;
  W = nW;
}

uint32_t Color::RGB(uint8_t brightness) {
  if (brightness != 255)
    return (scale8(R, brightness) << 16) | (scale8(G, brightness) << 8) | scale8(B, brightness);
  return (R << 16) | (G << 8) | B;
}

uint32_t Color::GRB(uint8_t brightness) {
  if (brightness != 255)
    return (scale8(G, brightness) << 16) | (scale8(R, brightness) << 8) | scale8(B, brightness);
  return (G << 16) | (R << 8) | B;
}

Color Color::Scale(uint8_t brightness) {
  return Color(scale8(R, brightness), scale8(G, brightness), scale8(B, brightness));
}

Color Color::ToLowBrightness(bool cancel, uint8_t scale) {
  if (!cancel)
  { return Scale(scale); }
  return Color(R, G, B, W);
}

uint8_t Color::scale8(uint8_t i, uint8_t scale) {
  return ((uint16_t)i * (uint16_t)scale) >> 8;
}

uint8_t Color::scale8_video(uint8_t i, uint8_t scale) {
  return (((uint16_t)i * (uint16_t)scale) >> 8) + ((i && scale) ? 1 : 0);
}

Color Color::HsvToRgb(float h, float s, float v) {
  uint8_t r = int(255 * v * mix(1.0, constrain(std::abs(fract(h + 1.0) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s));
  uint8_t g = int(255 * v * mix(1.0, constrain(std::abs(fract(h + 0.6666666) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s));
  uint8_t b = int(255 * v * mix(1.0, constrain(std::abs(fract(h + 0.3333333) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s));
  return Color(r, g, b);
}