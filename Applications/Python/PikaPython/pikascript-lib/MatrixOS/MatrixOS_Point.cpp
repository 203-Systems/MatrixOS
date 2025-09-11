#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // Point constructor
    void _MatrixOSPoint_Point___init__(PikaObj *self, PikaObj* x, PikaObj* y) {
        int x_val = obj_getInt(x, (char*)"value");
        int y_val = obj_getInt(y, (char*)"value");
        obj_setInt(self, (char*)"x", x_val);
        obj_setInt(self, (char*)"y", y_val);
    }

    // Point operators
    PikaObj* _MatrixOSPoint_Point___add__(PikaObj *self, PikaObj* other) {
        int x1 = obj_getInt(self, (char*)"x");
        int y1 = obj_getInt(self, (char*)"y");
        int x2 = obj_getInt(other, (char*)"x");
        int y2 = obj_getInt(other, (char*)"y");
        
        Point p1(x1, y1);
        Point p2(x2, y2);
        Point result = p1 + p2;
        
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", result.x);
        obj_setInt(new_point, (char*)"y", result.y);
        return new_point;
    }

    PikaObj* _MatrixOSPoint_Point___sub__(PikaObj *self, PikaObj* other) {
        int x1 = obj_getInt(self, (char*)"x");
        int y1 = obj_getInt(self, (char*)"y");
        int x2 = obj_getInt(other, (char*)"x");
        int y2 = obj_getInt(other, (char*)"y");
        
        Point p1(x1, y1);
        Point p2(x2, y2);
        Point result = p1 - p2;
        
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", result.x);
        obj_setInt(new_point, (char*)"y", result.y);
        return new_point;
    }

    PikaObj* _MatrixOSPoint_Point___mul__(PikaObj *self, int val) {
        int x = obj_getInt(self, (char*)"x");
        int y = obj_getInt(self, (char*)"y");
        
        Point p(x, y);
        Point result = p * val;
        
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", result.x);
        obj_setInt(new_point, (char*)"y", result.y);
        return new_point;
    }

    PikaObj* _MatrixOSPoint_Point___div__(PikaObj *self, int val) {
        int x = obj_getInt(self, (char*)"x");
        int y = obj_getInt(self, (char*)"y");
        
        Point p(x, y);
        Point result = p / val;
        
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", result.x);
        obj_setInt(new_point, (char*)"y", result.y);
        return new_point;
    }

    pika_bool _MatrixOSPoint_Point___eq__(PikaObj *self, PikaObj* other) {
        int x1 = obj_getInt(self, (char*)"x");
        int y1 = obj_getInt(self, (char*)"y");
        int x2 = obj_getInt(other, (char*)"x");
        int y2 = obj_getInt(other, (char*)"y");
        
        return (x1 == x2 && y1 == y2) ? pika_true : pika_false;
    }

    pika_bool _MatrixOSPoint_Point___ne__(PikaObj *self, PikaObj* other) {
        int x1 = obj_getInt(self, (char*)"x");
        int y1 = obj_getInt(self, (char*)"y");
        int x2 = obj_getInt(other, (char*)"x");
        int y2 = obj_getInt(other, (char*)"y");
        
        return (x1 != x2 || y1 != y2) ? pika_true : pika_false;
    }

    // Point methods
    PikaObj* _MatrixOSPoint_Point_Rotate(PikaObj *self, PikaObj* rotation, PikaObj* dimension, PikaObj* reverse) {
        int x = obj_getInt(self, (char*)"x");
        int y = obj_getInt(self, (char*)"y");
        int rotation_val = obj_getInt(rotation, (char*)"value");
        int dim_x = obj_getInt(dimension, (char*)"x");
        int dim_y = obj_getInt(dimension, (char*)"y");
        pika_bool reverse_val = obj_getBool(reverse, (char*)"value");
        
        Point p(x, y);
        Point dim(dim_x, dim_y);
        Point result = p.Rotate((Direction)rotation_val, dim, reverse_val != 0);
        
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", result.x);
        obj_setInt(new_point, (char*)"y", result.y);
        return new_point;
    }

    // Static method
    PikaObj* _MatrixOSPoint_Point_Invalid(PikaObj *self) {
        Point invalid = Point::Invalid();
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", invalid.x);
        obj_setInt(new_point, (char*)"y", invalid.y);
        return new_point;
    }
}