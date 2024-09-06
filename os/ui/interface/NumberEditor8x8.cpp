#include "MatrixOS.h"
#include "ui/UI.h"

namespace MatrixOS::UIInterface
{
  // Three digit max
  int32_t NumberSelector8x8(int32_t value, Color color, string name, int32_t lower_limit,
                            int32_t upper_limit, int32_t* custom_modifier) {
    Point origin = Point((Device::x_size - 1) / 2, (Device::y_size - 1) / 2);

    UI numberSelector(name, color);

    UI4pxNumber numberDisplay(color, 3, (int32_t*)&value, Color(0xFFFFFF));
    numberSelector.AddUIComponent(numberDisplay, origin + Point(-4, -3));

    int32_t modifier[8] = {-50, -20, -5, -1, 1, 5, 20, 50};
    uint8_t gradient[8] = {255, 127, 64, 32, 32, 64, 127, 255};
    UINumberModifier numberModifier(color, 8, (int32_t*)&value, custom_modifier != nullptr ? custom_modifier : modifier, gradient, lower_limit, upper_limit);
    numberSelector.AddUIComponent(numberModifier, origin + Point(-3, 4));

    numberSelector.Start();
    return value;
  }
}
