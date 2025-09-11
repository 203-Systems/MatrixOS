#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // Color constructor
    void _MatrixOSColor_Color___init__(PikaObj *self, int r, int g, int b, int w) {
        obj_setInt(self, (char*)"r", r);
        obj_setInt(self, (char*)"g", g);
        obj_setInt(self, (char*)"b", b);
        obj_setInt(self, (char*)"w", w);
    }

    // Color property getter
    int MatrixOSColor_Color_RGB(PikaObj *self) {
        return  (uint32_t)obj_getInt(self, (char*)"w") << 24 |
                (uint32_t)obj_getInt(self, (char*)"r") << 16 |
                (uint32_t)obj_getInt(self, (char*)"g") << 8  |
                (uint32_t)obj_getInt(self, (char*)"b");
    }

    // Color manipulation methods
    PikaObj* MatrixOSColor_Color_Dim(PikaObj *self, pika_float factor) {
        int r = obj_getInt(self, (char*)"r");
        int g = obj_getInt(self, (char*)"g");
        int b = obj_getInt(self, (char*)"b");
        int w = obj_getInt(self, (char*)"w");

        uint8_t factor_u8 = (uint8_t)(factor * 255);
        Color color(r, g, b, w);
        Color dimmed = color.Dim(factor_u8);

        PikaObj* new_color = newNormalObj(New_PikaObj);

        obj_setInt(new_color, (char*)"r", dimmed.R);
        obj_setInt(new_color, (char*)"g", dimmed.G);
        obj_setInt(new_color, (char*)"b", dimmed.B);
        obj_setInt(new_color, (char*)"w", dimmed.W);
        return new_color;
    }

    PikaObj* MatrixOSColor_Color_DimIfNot(PikaObj *self, pika_bool not_dim, pika_float factor) {
        if (not_dim) {
            return self;
        } else {
            // Dim the color
            return MatrixOSColor_Color_Dim(self, factor);
        }
    }

    PikaObj* MatrixOSColor_Color_Scale(PikaObj *self, pika_float factor) {
        int r = obj_getInt(self, (char*)"r");
        int g = obj_getInt(self, (char*)"g");
        int b = obj_getInt(self, (char*)"b");
        int w = obj_getInt(self, (char*)"w");

        Color color(r, g, b, w);
        Color scaled = color.Scale(factor);
        
        PikaObj* new_color = newNormalObj(New_PikaObj);
        obj_setInt(new_color, (char*)"r", scaled.R);
        obj_setInt(new_color, (char*)"g", scaled.G);
        obj_setInt(new_color, (char*)"b", scaled.B);
        obj_setInt(new_color, (char*)"w", scaled.W);
        return new_color;
    }

    // Color operators
    pika_bool MatrixOSColor_Color___eq__(PikaObj *self, PikaObj* other) {
        int rgb1 = obj_getInt(self, (char*)"value");
        int rgb2 = obj_getInt(other, (char*)"value");
        return (rgb1 == rgb2) ? pika_true : pika_false;
    }

    pika_bool MatrixOSColor_Color___ne__(PikaObj *self, PikaObj* other) {
        int rgb1 = obj_getInt(self, (char*)"value");
        int rgb2 = obj_getInt(other, (char*)"value");
        return (rgb1 != rgb2) ? pika_true : pika_false;
    }
}