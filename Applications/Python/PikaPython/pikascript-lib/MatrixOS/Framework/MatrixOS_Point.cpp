#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // Point constructor
    void _MatrixOS_Point_Point___init__(PikaObj *self, int x, int y) {
        obj_setInt(self, (char*)"x", x);
        obj_setInt(self, (char*)"y", y);
    }

    // Point operators
    PikaObj* _MatrixOS_Point_Point___add__(PikaObj *self, PikaObj* other) {
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

    PikaObj* _MatrixOS_Point_Point___sub__(PikaObj *self, PikaObj* other) {
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

    PikaObj* _MatrixOS_Point_Point___mul__(PikaObj *self, int val) {
        int x = obj_getInt(self, (char*)"x");
        int y = obj_getInt(self, (char*)"y");
        
        Point p(x, y);
        Point result = p * val;
        
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", result.x);
        obj_setInt(new_point, (char*)"y", result.y);
        return new_point;
    }

    PikaObj* _MatrixOS_Point_Point___div__(PikaObj *self, int val) {
        int x = obj_getInt(self, (char*)"x");
        int y = obj_getInt(self, (char*)"y");
        
        Point p(x, y);
        Point result = p / val;
        
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", result.x);
        obj_setInt(new_point, (char*)"y", result.y);
        return new_point;
    }

    pika_bool _MatrixOS_Point_Point___eq__(PikaObj *self, PikaObj* other) {
        int x1 = obj_getInt(self, (char*)"x");
        int y1 = obj_getInt(self, (char*)"y");
        int x2 = obj_getInt(other, (char*)"x");
        int y2 = obj_getInt(other, (char*)"y");
        
        return (x1 == x2 && y1 == y2);
    }

    pika_bool _MatrixOS_Point_Point___ne__(PikaObj *self, PikaObj* other) {
        int x1 = obj_getInt(self, (char*)"x");
        int y1 = obj_getInt(self, (char*)"y");
        int x2 = obj_getInt(other, (char*)"x");
        int y2 = obj_getInt(other, (char*)"y");
        
        return (x1 != x2 || y1 != y2);
    }

    // Point methods
    PikaObj* _MatrixOS_Point_Point_Rotate(PikaObj *self, int rotation, PikaObj* dimension, pika_bool reverse) {
        int x = obj_getInt(self, (char*)"x");
        int y = obj_getInt(self, (char*)"y");
        int dim_x = obj_getInt(dimension, (char*)"x");
        int dim_y = obj_getInt(dimension, (char*)"y");
        
        Point p(x, y);
        Point dim(dim_x, dim_y);
        Point result = p.Rotate((Direction)rotation, dim, reverse != 0);
        
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", result.x);
        obj_setInt(new_point, (char*)"y", result.y);
        return new_point;
    }

    // Static methods
    PikaObj* _MatrixOS_Point_Point_Invalid(PikaObj *self) {
        Point invalid = Point::Invalid();
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", invalid.x);
        obj_setInt(new_point, (char*)"y", invalid.y);
        return new_point;
    }

    PikaObj* _MatrixOS_Point_Point_Origin(PikaObj *self) {
        PikaObj* new_point = newNormalObj(New_PikaObj);
        obj_setInt(new_point, (char*)"x", 0);
        obj_setInt(new_point, (char*)"y", 0);
        return new_point;
    }
}