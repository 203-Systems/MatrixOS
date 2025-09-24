#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    // PikaObj constructor
    PikaObj* New__MatrixOS_Dimension_Dimension(Args *args);

    // Dimension constructor
    void _MatrixOS_Dimension_Dimension___init__(PikaObj *self, PikaTuple* val) {
        uint8_t args_size = pikaTuple_getSize(val);
        
        if(args_size == 1)
        {
            // Single argument - raw bytes
            Arg* arg = pikaTuple_getArg(val, 0);
            int rawByte = arg_getInt(arg);
            createCppObjPtrInPikaObj<Dimension>(self, (uint32_t)rawByte);
        }
        else if(args_size == 2)
        {
            // Two arguments - x and y
            Arg* x_arg = pikaTuple_getArg(val, 0);
            Arg* y_arg = pikaTuple_getArg(val, 1);
            
            int x = arg_getInt(x_arg);
            int y = arg_getInt(y_arg);
            
            createCppObjPtrInPikaObj<Dimension>(self, (int16_t)x, (int16_t)y);
        }
        else
        {
            // Default to zero dimension
            createCppObjPtrInPikaObj<Dimension>(self);
        }
    }

    // Getters
    int _MatrixOS_Dimension_Dimension_X(PikaObj *self) {
        Dimension* dimension = getCppObjPtrInPikaObj<Dimension>(self);
        if (!dimension) return 0;
        return dimension->x;
    }

    int _MatrixOS_Dimension_Dimension_Y(PikaObj *self) {
        Dimension* dimension = getCppObjPtrInPikaObj<Dimension>(self);
        if (!dimension) return 0;
        return dimension->y;
    }

    // Setters
    void _MatrixOS_Dimension_Dimension_SetX(PikaObj *self, int x) {
        Dimension* dimension = getCppObjPtrInPikaObj<Dimension>(self);
        if (!dimension) return;
        dimension->x = (int16_t)x;
    }

    void _MatrixOS_Dimension_Dimension_SetY(PikaObj *self, int y) {
        Dimension* dimension = getCppObjPtrInPikaObj<Dimension>(self);
        if (!dimension) return;
        dimension->y = (int16_t)y;
    }

    // Methods
    pika_bool _MatrixOS_Dimension_Dimension_Contains(PikaObj *self, PikaObj* point) {
        Dimension* dimension = getCppObjPtrInPikaObj<Dimension>(self);
        Point* pointPtr = getCppObjPtrInPikaObj<Point>(point);
        
        if (!dimension || !pointPtr) return false;
        
        return dimension->Contains(*pointPtr);
    }

    int _MatrixOS_Dimension_Dimension_Area(PikaObj *self) {
        Dimension* dimension = getCppObjPtrInPikaObj<Dimension>(self);
        if (!dimension) return 0;
        return dimension->Area();
    }

    // Operators
    PikaObj* _MatrixOS_Dimension_Dimension___add__(PikaObj *self, PikaObj* other) {
        Dimension* dimension1 = getCppObjPtrInPikaObj<Dimension>(self);
        Dimension* dimension2 = getCppObjPtrInPikaObj<Dimension>(other);
        
        if (!dimension1 || !dimension2) return nullptr;
        
        Dimension result = *dimension1 + *dimension2;
        
        PikaObj* new_dimension = newNormalObj(New__MatrixOS_Dimension_Dimension);
        copyCppObjIntoPikaObj<Dimension>(new_dimension, result);
        return new_dimension;
    }

    pika_bool _MatrixOS_Dimension_Dimension___eq__(PikaObj *self, PikaObj* other) {
        Dimension* dimension1 = getCppObjPtrInPikaObj<Dimension>(self);
        Dimension* dimension2 = getCppObjPtrInPikaObj<Dimension>(other);
        
        if (!dimension1 || !dimension2) return false;
        
        return !(*dimension1 != *dimension2);
    }


    pika_bool _MatrixOS_Dimension_Dimension___bool__(PikaObj *self) {
        Dimension* dimension = getCppObjPtrInPikaObj<Dimension>(self);
        if (!dimension) return false;
        return (bool)*dimension;
    }

}