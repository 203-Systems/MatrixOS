  #pragma once
  #include "MatrixOS.h"
  
  namespace MatrixOS::UIInterface
  {
    void TextScroll(string ascii, Color color, uint16_t speed = 10, bool loop = false);
    int32_t NumberSelector8x8(int32_t value, Color color, string name, int32_t lower_limit, int32_t upper_limit);
    bool ColorPicker(Color& color);
  }