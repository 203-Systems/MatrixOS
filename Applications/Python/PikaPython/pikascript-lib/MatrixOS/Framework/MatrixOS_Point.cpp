#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    PikaObj* New__MatrixOS_Point_Point(Args *args);

    // Point constructor
    void _MatrixOS_Point_Point___init__(PikaObj *self, int x, int y) {
        createCppObjPtrInPikaObj<Point>(self, x, y);
    }

    // Point getter methods
    int _MatrixOS_Point_Point_X(PikaObj *self) {
        Point* point = getCppObjPtrInPikaObj<Point>(self);
        if (!point) return 0;
        return point->x;
    }

    int _MatrixOS_Point_Point_Y(PikaObj *self) {
        Point* point = getCppObjPtrInPikaObj<Point>(self);
        if (!point) return 0;
        return point->y;
    }

    // Point setter methods
    void _MatrixOS_Point_Point_SetX(PikaObj *self, int x) {
        Point* point = getCppObjPtrInPikaObj<Point>(self);
        if (!point) return;
        point->x = x;
    }

    void _MatrixOS_Point_Point_SetY(PikaObj *self, int y) {
        Point* point = getCppObjPtrInPikaObj<Point>(self);
        if (!point) return;
        point->y = y;
    }

    // Point operators
    PikaObj* _MatrixOS_Point_Point___add__(PikaObj *self, PikaObj* other) {
        Point* p1 = getCppObjPtrInPikaObj<Point>(self);
        Point* p2 = getCppObjPtrInPikaObj<Point>(other);

        Point result;
        if (!p1 || !p2) {
            result = Point::Invalid();
        }
        else
        {
            result = *p1 + *p2;
        }


        PikaObj* new_point = newNormalObj(New__MatrixOS_Point_Point);
        copyCppObjIntoPikaObj<Point>(new_point, result);

        return new_point;
    }

    PikaObj* _MatrixOS_Point_Point___sub__(PikaObj *self, PikaObj* other) {
        Point* p1 = getCppObjPtrInPikaObj<Point>(self);
        Point* p2 = getCppObjPtrInPikaObj<Point>(other);

        Point result;
        if (!p1 || !p2) {
            result = Point::Invalid();
        }
        else
        {
            result = *p1 - *p2;
        }

        PikaObj* new_point = newNormalObj(New__MatrixOS_Point_Point);
        copyCppObjIntoPikaObj<Point>(new_point, result);
        return new_point;
    }

    PikaObj* _MatrixOS_Point_Point___mul__(PikaObj *self, int val) {
        Point* p = getCppObjPtrInPikaObj<Point>(self);

        Point result;
        if (!p) {
            result = Point::Invalid();
        }
        else
        {
            result = *p * val;
        }

        PikaObj* new_point = newNormalObj(New__MatrixOS_Point_Point);
        copyCppObjIntoPikaObj<Point>(new_point, result);
        return new_point;
    }

    pika_bool _MatrixOS_Point_Point___eq__(PikaObj *self, PikaObj* other) {
        Point* p1 = getCppObjPtrInPikaObj<Point>(self);
        Point* p2 = getCppObjPtrInPikaObj<Point>(other);
        if (!p1 || !p2) return false;

        return *p1 == *p2;
    }

    pika_bool _MatrixOS_Point_Point___ne__(PikaObj *self, PikaObj* other) {
        return !_MatrixOS_Point_Point___eq__(self, other);
    }

    // Point methods
    PikaObj* _MatrixOS_Point_Point_Rotate(PikaObj *self, int rotation, PikaObj* dimension, pika_bool reverse) {
        Point* p = getCppObjPtrInPikaObj<Point>(self);
        Point* dim = getCppObjPtrInPikaObj<Point>(dimension);

        Point result;
        if (!p || !dim) {
            result = Point::Invalid();
        }
        else
        {
            result = p->Rotate((Direction)rotation, *dim, reverse != 0);
        }

        PikaObj* new_point = newNormalObj(New__MatrixOS_Point_Point);
        copyCppObjIntoPikaObj<Point>(new_point, result);
        return new_point;
    }

    // Static methods
    PikaObj* _MatrixOS_Point_Point_Invalid(PikaObj *self) {
        Point invalid = Point::Invalid();
        PikaObj* new_point = newNormalObj(New__MatrixOS_Point_Point);
        copyCppObjIntoPikaObj<Point>(new_point, invalid);
        return new_point;
    }

    PikaObj* _MatrixOS_Point_Point_Origin(PikaObj *self) {
        Point origin(0, 0);
        PikaObj* new_point = newNormalObj(New__MatrixOS_Point_Point);
        copyCppObjIntoPikaObj<Point>(new_point, origin);
        return new_point;
    }
}