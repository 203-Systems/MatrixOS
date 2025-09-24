#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    // PikaObj constructor
    PikaObj* New__MatrixOS_Color_Color(Args *args);

    // Color constructor
    void _MatrixOS_Color_Color___init__(PikaObj *self, PikaTuple* val) {
        uint8_t args_size = pikaTuple_getSize(val);
        
        if(args_size == 1)
        {
            // Single argument - could be hex value
            Arg* arg = pikaTuple_getArg(val, 0);
            int hex = arg_getInt(arg);
            createCppObjPtrInPikaObj<Color>(self, (uint32_t)hex);
        }
        else if(args_size == 3)
        {
            // RGB constructor
            Arg* r_arg = pikaTuple_getArg(val, 0);
            Arg* g_arg = pikaTuple_getArg(val, 1);
            Arg* b_arg = pikaTuple_getArg(val, 2);
            
            int r = arg_getInt(r_arg);
            int g = arg_getInt(g_arg);
            int b = arg_getInt(b_arg);
            
            createCppObjPtrInPikaObj<Color>(self, (uint8_t)r, (uint8_t)g, (uint8_t)b);
        }
        else if(args_size == 4)
        {
            // RGBW constructor
            Arg* r_arg = pikaTuple_getArg(val, 0);
            Arg* g_arg = pikaTuple_getArg(val, 1);
            Arg* b_arg = pikaTuple_getArg(val, 2);
            Arg* w_arg = pikaTuple_getArg(val, 3);
            
            int r = arg_getInt(r_arg);
            int g = arg_getInt(g_arg);
            int b = arg_getInt(b_arg);
            int w = arg_getInt(w_arg);
            
            createCppObjPtrInPikaObj<Color>(self, (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)w);
        }
        else
        {
            // Black
            createCppObjPtrInPikaObj<Color>(self, 0);
        }
    }

    PikaObj* _MatrixOS_Color_Color_FromRGBW(PikaObj *self, int r, int g, int b, int w) {
        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        createCppObjPtrInPikaObj<Color>(new_color, (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)w);
        return new_color;
    }
    
    PikaObj* _MatrixOS_Color_Color_FromRGB(PikaObj *self, int r, int g, int b) {
        return _MatrixOS_Color_Color_FromRGBW(self, r, g, b, 0);
    }
    
    PikaObj* _MatrixOS_Color_Color_FromHex(PikaObj *self, int wrgb) {
        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        createCppObjPtrInPikaObj<Color>(new_color, (uint32_t)wrgb);
        return new_color;
    }

    // Color getter methods
    int _MatrixOS_Color_Color_R(PikaObj *self) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return 0;
        return color->R;
    }

    int _MatrixOS_Color_Color_G(PikaObj *self) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return 0;
        return color->G;
    }

    int _MatrixOS_Color_Color_B(PikaObj *self) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return 0;
        return color->B;
    }

    int _MatrixOS_Color_Color_W(PikaObj *self) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return 0;
        return color->W;
    }

    // Color setter methods
    void _MatrixOS_Color_Color_SetR(PikaObj *self, int r) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return;
        color->R = (uint8_t)r;
    }

    void _MatrixOS_Color_Color_SetG(PikaObj *self, int g) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return;
        color->G = (uint8_t)g;
    }

    void _MatrixOS_Color_Color_SetB(PikaObj *self, int b) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return;
        color->B = (uint8_t)b;
    }

    void _MatrixOS_Color_Color_SetW(PikaObj *self, int w) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return;
        color->W = (uint8_t)w;
    }

    int _MatrixOS_Color_Color_RGB(PikaObj *self) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return 0;
        return color->RGB();
    }

    int _MatrixOS_Color_Color_WRGB(PikaObj *self) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return 0;
        return (color->W << 24) | (color->R << 16) | (color->G << 8) | color->B;
    }

    // Color manipulation methods
    PikaObj* _MatrixOS_Color_Color_Dim(PikaObj *self, pika_float factor) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return nullptr;
        
        uint8_t factor_u8 = (uint8_t)(factor * 255);
        Color dimmed = color->Dim(factor_u8);

        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        copyCppObjIntoPikaObj<Color>(new_color, dimmed);
        return new_color;
    }

    PikaObj* _MatrixOS_Color_Color_DimIfNot(PikaObj *self, pika_bool not_dim, pika_float factor) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return nullptr;
        
        if (not_dim) {
            return self;
        } else {
            uint8_t factor_u8 = (uint8_t)(factor * 255);
            Color dimmed = color->DimIfNot(false, factor_u8);
            
            PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
            copyCppObjIntoPikaObj<Color>(new_color, dimmed);
            return new_color;
        }
    }

    PikaObj* _MatrixOS_Color_Color_Scale(PikaObj *self, pika_float factor) {
        Color* color = getCppObjPtrInPikaObj<Color>(self);
        if (!color) return nullptr;
        
        Color scaled = color->Scale((uint8_t)(factor * 255));
        
        PikaObj* new_color = newNormalObj(New__MatrixOS_Color_Color);
        copyCppObjIntoPikaObj<Color>(new_color, scaled);
        return new_color;
    }

    // Color operators
    pika_bool _MatrixOS_Color_Color___eq__(PikaObj *self, PikaObj* other) {
        Color* color1 = getCppObjPtrInPikaObj<Color>(self);
        Color* color2 = getCppObjPtrInPikaObj<Color>(other);

        if (!color1 || !color2) return false;
        
        return *color1 == *color2;
    }

    pika_bool _MatrixOS_Color_Color___ne__(PikaObj *self, PikaObj* other) {
        return !_MatrixOS_Color_Color___eq__(self, other);
    }
}