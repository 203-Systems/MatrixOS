#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // Color constructor
    void _MatrixOS_Color_Color___init__(PikaObj *self, int r, int g, int b, int w) {
        obj_setInt(self, (char*)"r", r);
        obj_setInt(self, (char*)"g", g);
        obj_setInt(self, (char*)"b", b);
        obj_setInt(self, (char*)"w", w);
    }

    PikaObj* _MatrixOS_Color_Color_FromRGBW(PikaObj *self, int r, int g, int b, int w) {
        PikaObj* new_color = newNormalObj(New_PikaObj);

        obj_setInt(new_color, (char*)"r", r);
        obj_setInt(new_color, (char*)"g", g);
        obj_setInt(new_color, (char*)"b", b);
        obj_setInt(new_color, (char*)"w", w);

        return new_color;
    }
    
    PikaObj* _MatrixOS_Color_Color_FromRGB(PikaObj *self, int r, int g, int b) {
        return _MatrixOS_Color_Color_FromRGBW(self, r, g, b, 0);
    }
    
    PikaObj* _MatrixOS_Color_Color_FromHex(PikaObj *self, int hex) {
        // Extract WRGB components from 32-bit hex value
        int w = (hex >> 24) & 0xFF;
        int r = (hex >> 16) & 0xFF;
        int g = (hex >> 8) & 0xFF;
        int b = hex & 0xFF;

        return _MatrixOS_Color_Color_FromRGBW(self, r, g, b, w);
    }

    // Color manipulation methods
    PikaObj* _MatrixOS_Color_Color_Dim(PikaObj *self, pika_float factor) {
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

    PikaObj* _MatrixOS_Color_Color_DimIfNot(PikaObj *self, pika_bool not_dim, pika_float factor) {
        if (not_dim) {
            return self;
        } else {
            // Dim the color
            return _MatrixOS_Color_Color_Dim(self, factor);
        }
    }

    PikaObj* _MatrixOS_Color_Color_Scale(PikaObj *self, pika_float factor) {
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
    pika_bool _MatrixOS_Color_Color___eq__(PikaObj *self, PikaObj* other) {
        int r1 = obj_getInt(self, (char*)"r");
        int g1 = obj_getInt(self, (char*)"g");
        int b1 = obj_getInt(self, (char*)"b");
        int w1 = obj_getInt(self, (char*)"w");
        
        int r2 = obj_getInt(other, (char*)"r");
        int g2 = obj_getInt(other, (char*)"g");
        int b2 = obj_getInt(other, (char*)"b");
        int w2 = obj_getInt(other, (char*)"w");
        
        return (r1 == r2 && g1 == g2 && b1 == b2 && w1 == w2);
    }

    pika_bool _MatrixOS_Color_Color___ne__(PikaObj *self, PikaObj* other) {
        return _MatrixOS_Color_Color___eq__(self, other) ? pika_false : pika_true;
    }
}