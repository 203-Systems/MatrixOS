#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    // PikaObj constructor for Color
    PikaObj* New__MatrixOS_Color_Color(Args *args);

    // Rainbow effect - generates a rainbow color
    PikaObj* _MatrixOS_ColorEffects_Rainbow(PikaObj *self, int period, int offset) {
        Color color = ColorEffects::Rainbow((uint16_t)period, (int32_t)offset);
        
        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        copyCppObjIntoPikaObj<Color>(new_color, color);
        return new_color;
    }

    // Breath effect - returns brightness value 0-255
    int _MatrixOS_ColorEffects_Breath(PikaObj *self, int period, int offset) {
        return ColorEffects::Breath((uint16_t)period, (int32_t)offset);
    }

    // BreathLowBound effect - returns brightness value with lower bound
    int _MatrixOS_ColorEffects_BreathLowBound(PikaObj *self, int low_bound, int period, int offset) {
        return ColorEffects::BreathLowBound((uint8_t)low_bound, (uint16_t)period, (int32_t)offset);
    }

    // Strobe effect - returns brightness value 0 or 255
    int _MatrixOS_ColorEffects_Strobe(PikaObj *self, int period, int offset) {
        return ColorEffects::Strobe((uint16_t)period, (int32_t)offset);
    }

    // Saw effect - returns brightness value 0-255
    int _MatrixOS_ColorEffects_Saw(PikaObj *self, int period, int offset) {
        return ColorEffects::Saw((uint16_t)period, (int32_t)offset);
    }

    // Triangle effect - returns brightness value 0-255
    int _MatrixOS_ColorEffects_Triangle(PikaObj *self, int period, int offset) {
        return ColorEffects::Triangle((uint16_t)period, (int32_t)offset);
    }

    // ColorBreath effect - applies breath effect to a color
    PikaObj* _MatrixOS_ColorEffects_ColorBreath(PikaObj *self, PikaObj* color_obj, int period, int offset) {
        Color* color = getCppObjPtrInPikaObj<Color>(color_obj);
        if (!color) return nullptr;
        
        Color result = ColorEffects::ColorBreath(*color, (uint16_t)period, (int32_t)offset);
        
        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        copyCppObjIntoPikaObj<Color>(new_color, result);
        return new_color;
    }

    // ColorBreathLowBound effect - applies breath with lower bound to a color
    PikaObj* _MatrixOS_ColorEffects_ColorBreathLowBound(PikaObj *self, PikaObj* color_obj, int low_bound, int period, int offset) {
        Color* color = getCppObjPtrInPikaObj<Color>(color_obj);
        if (!color) return nullptr;
        
        Color result = ColorEffects::ColorBreathLowBound(*color, (uint8_t)low_bound, (uint16_t)period, (int32_t)offset);
        
        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        copyCppObjIntoPikaObj<Color>(new_color, result);
        return new_color;
    }

    // ColorStrobe effect - applies strobe effect to a color
    PikaObj* _MatrixOS_ColorEffects_ColorStrobe(PikaObj *self, PikaObj* color_obj, int period, int offset) {
        Color* color = getCppObjPtrInPikaObj<Color>(color_obj);
        if (!color) return nullptr;
        
        Color result = ColorEffects::ColorStrobe(*color, (uint16_t)period, (int32_t)offset);
        
        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        copyCppObjIntoPikaObj<Color>(new_color, result);
        return new_color;
    }

    // ColorSaw effect - applies sawtooth effect to a color
    PikaObj* _MatrixOS_ColorEffects_ColorSaw(PikaObj *self, PikaObj* color_obj, int period, int offset) {
        Color* color = getCppObjPtrInPikaObj<Color>(color_obj);
        if (!color) return nullptr;
        
        Color result = ColorEffects::ColorSaw(*color, (uint16_t)period, (int32_t)offset);
        
        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        copyCppObjIntoPikaObj<Color>(new_color, result);
        return new_color;
    }

    // ColorTriangle effect - applies triangle wave effect to a color
    PikaObj* _MatrixOS_ColorEffects_ColorTriangle(PikaObj *self, PikaObj* color_obj, int period, int offset) {
        Color* color = getCppObjPtrInPikaObj<Color>(color_obj);
        if (!color) return nullptr;
        
        Color result = ColorEffects::ColorTriangle(*color, (uint16_t)period, (int32_t)offset);
        
        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        copyCppObjIntoPikaObj<Color>(new_color, result);
        return new_color;
    }
}