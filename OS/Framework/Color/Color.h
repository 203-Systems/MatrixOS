#pragma once

#include <stdint.h>
#include "Fract16.h"

#define COLOR_LOW_STATE_SCALE 56

class Color {
 public:
  uint8_t R = 0;
  uint8_t G = 0;
  uint8_t B = 0;
  uint8_t W = 0;

  Color();
  Color(uint32_t WRGB);
  Color(uint8_t nR, uint8_t nG, uint8_t nB, uint8_t nW = 0);

  uint32_t RGB(uint8_t brightness = 255) const;
  uint32_t GRB(uint8_t brightness = 255) const;
  Color Scale(uint8_t scale) const;
  Color Dim(uint8_t scale = COLOR_LOW_STATE_SCALE) const;
  Color DimIfNot(bool not_dim = false, uint8_t scale = COLOR_LOW_STATE_SCALE) const;  // Helper for UI, make ui variable
                                                                                      // as parameter so the output                                                                // dynamically change based on the                                                                   // variable
  Color Gamma() const;

  static uint8_t scale8(uint8_t i, uint8_t scale);

  // A special type of scale. It ensures value won't be 0 after the scale.
  static uint8_t scale8_video(uint8_t i, uint8_t scale);

  static Color HsvToRgb(float h, float s, float v);
  static void RgbToHsv(Color rgb, float* h, float* s, float* v);

  static Color Crossfade(Color color1, Color color2, Fract16 ratio);

  // Equality comparison operators
  bool operator==(const Color& other) const {
    return R == other.R && G == other.G && B == other.B && W == other.W;
  }

  bool operator!=(const Color& other) const {
    return !(*this == other);
  }

  operator bool() { return R || G || B || W; }
};

const uint8_t led_gamma[256] = {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 14, 14, 15, 15, 16, 17, 17, 18, 18, 19, 20, 20, 21, 22, 22, 23, 24, 24, 25, 26, 26, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 43, 44, 45, 46, 47, 48, 49, 49, 50, 51, 52, 53, 54, 55, 56, 57, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 107, 108, 109, 110, 111, 112, 113, 115, 116, 117, 118, 119, 120, 122, 123, 124, 125, 126, 127, 129, 130, 131, 132, 133, 135, 136, 137, 138, 140, 141, 142, 143, 144, 146, 147, 148, 149, 151, 152, 153, 155, 156, 157, 158, 160, 161, 162, 164, 165, 166, 167, 169, 170, 171, 173, 174, 175, 177, 178, 179, 181, 182, 183, 185, 186, 187, 189, 190, 191, 193, 194, 196, 197, 198, 200, 201, 202, 204, 205, 207, 208, 209, 211, 212, 214, 215, 217, 218, 219, 221, 222, 224, 225, 227, 228, 229, 231, 232, 234, 235, 237, 238, 240, 241, 243, 244, 246, 247, 249, 250, 252, 253, 255};