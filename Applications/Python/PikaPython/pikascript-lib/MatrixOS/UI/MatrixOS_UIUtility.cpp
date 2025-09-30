#include "MatrixOS.h"
#include "UI/UIUtilities.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    // UIUtilities implementation
    PikaObj* New__MatrixOS_Color_Color(Args *args);

    void _MatrixOS_UIUtility_TextScroll(PikaObj *self, char* text, PikaObj* color, int speed, pika_bool loop) {
        // Get Color object from PikaObj
        Color* color_ptr = getCppObjPtrInPikaObj<Color>(color);
        if (!color_ptr) return;

        MatrixOS::UIUtility::TextScroll(string(text), *color_ptr, speed, loop);
    }

    int _MatrixOS_UIUtility_NumberSelector8x8(PikaObj *self, int value, PikaObj* color, char* name, int lower_limit, int upper_limit) {
        // Get Color object from PikaObj
        Color* color_ptr = getCppObjPtrInPikaObj<Color>(color);
        if (!color_ptr) return value; // Return original value if color is invalid

        return MatrixOS::UIUtility::NumberSelector8x8(value, *color_ptr, string(name), lower_limit, upper_limit);
    }

    Arg* _MatrixOS_UIUtility_ColorPicker(PikaObj *self) {
        Color picked_color(0);

        bool success = MatrixOS::UIUtility::ColorPicker(picked_color);

        if (success) {
            // Create new Color object and return it
            PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
            copyCppObjIntoPikaObj<Color>(new_color, picked_color);
            return arg_newObj(new_color);
        }
        return arg_newNone();
    }
}