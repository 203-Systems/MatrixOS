#pragma once
#include "MatrixOS.h"
#include <climits>

namespace MatrixOS::UIUtility
{
void TextScroll(string ascii, Color color, uint16_t speed = 10, bool loop = false);
int32_t NumberSelector8x8(int32_t value, Color color, string name, int32_t lowerLimit = INT_MIN, int32_t upperLimit = INT_MAX,
                          int32_t* customModifier = nullptr);
bool ColorPicker(Color& color, bool shade = true);
} // namespace MatrixOS::UIUtility