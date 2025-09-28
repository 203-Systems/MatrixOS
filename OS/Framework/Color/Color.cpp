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

uint32_t Color::RGB(uint8_t brightness) const {
  if (brightness != 255)
    return (scale8_video(R, brightness) << 16) | (scale8_video(G, brightness) << 8) | scale8_video(B, brightness); // Use scale_video to ensure it doesn't get completely removed
  return (R << 16) | (G << 8) | B;
}

uint32_t Color::GRB(uint8_t brightness) const {
  if (brightness != 255)
    return (scale8_video(G, brightness) << 16) | (scale8_video(R, brightness) << 8) | scale8_video(B, brightness); // Use scale_video to ensure it doesn't get completely removed
  return (G << 16) | (R << 8) | B;
}

Color Color::Scale(uint8_t brightness) const {
  return Color(scale8_video(R, brightness), scale8_video(G, brightness), scale8_video(B, brightness)); // Use scale_video to ensure it doesn't get completely removed
}
Color Color::Dim(uint8_t scale) const {
  return Scale(scale); 
}

Color Color::DimIfNot(bool not_dim, uint8_t scale) const {
  if (!not_dim)
  { return Scale(scale); }
  return Color(R, G, B, W);
}

Color Color::Gamma() const {
  return Color(led_gamma[R], led_gamma[G], led_gamma[B]);
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

void Color::RgbToHsv(Color rgb, float* h, float* s, float* v)
{
  float r = rgb.R / 255.0;
  float g = rgb.G / 255.0;
  float b = rgb.B / 255.0;

  float max = std::max(r, std::max(g, b));
  float min = std::min(r, std::min(g, b));

  *v = max;
  float delta = max - min;
  if (max != 0)
    *s = delta / max;
  else
  {
    // r = g = b = 0
    *s = 0;
    *h = -1;
    return;
  }
  if (r == max)
    *h = (g - b) / delta; // between yellow & magenta
  else if (g == max)
    *h = 2 + (b - r) / delta; // between cyan & yellow
  else
    *h = 4 + (r - g) / delta; // between magenta & cyan
  *h *= 1.0/6; // degrees
  if (*h < 0)
    *h += 1.0;
}
Color Color::Crossfade(Color color1, Color color2, Fract16 ratio) {
  uint8_t r = ratio.to8bits();
  uint8_t newR = (color1.R * (255 - r) + color2.R * r) >> 8;
  uint8_t newG = (color1.G * (255 - r) + color2.G * r) >> 8;
  uint8_t newB = (color1.B * (255 - r) + color2.B * r) >> 8;
  return Color(newR, newG, newB);
}
