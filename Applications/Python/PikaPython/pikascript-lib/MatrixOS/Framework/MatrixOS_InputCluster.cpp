#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

// Populate a Python InputCluster object from a C++ InputCluster (snapshot copy).
// Uses Pika attributes for all fields so the Python object is fully self-contained.
void populateInputClusterInPikaObj(PikaObj* obj, const InputCluster& c) {
    obj_setInt(obj, (char*)"_clusterId", c.clusterId);
    obj_setStr(obj, (char*)"_name", c.name.c_str());
    obj_setInt(obj, (char*)"_inputClass", (int)c.inputClass);
    obj_setInt(obj, (char*)"_shape", (int)c.shape);
    obj_setInt(obj, (char*)"_rootPointX", c.rootPoint.x);
    obj_setInt(obj, (char*)"_rootPointY", c.rootPoint.y);
    obj_setInt(obj, (char*)"_dimensionX", c.dimension.x);
    obj_setInt(obj, (char*)"_dimensionY", c.dimension.y);
    obj_setInt(obj, (char*)"_inputCount", c.inputCount);
    obj_setInt(obj, (char*)"_hasRootPoint", c.HasRootPoint() ? 1 : 0);
    obj_setInt(obj, (char*)"_hasCoordinates", c.HasCoordinates() ? 1 : 0);
}

extern "C" {
    PikaObj* New__MatrixOS_InputCluster_InputCluster(Args *args);
    PikaObj* New__MatrixOS_Point_Point(Args *args);
    PikaObj* New__MatrixOS_Dimension_Dimension(Args *args);

    void _MatrixOS_InputCluster_InputCluster___init__(PikaObj *self) {
        obj_setInt(self, (char*)"_clusterId", 0);
        obj_setStr(self, (char*)"_name", "");
        obj_setInt(self, (char*)"_inputClass", 0);
        obj_setInt(self, (char*)"_shape", 0);
        obj_setInt(self, (char*)"_rootPointX", INT16_MIN);
        obj_setInt(self, (char*)"_rootPointY", INT16_MIN);
        obj_setInt(self, (char*)"_dimensionX", 0);
        obj_setInt(self, (char*)"_dimensionY", 0);
        obj_setInt(self, (char*)"_inputCount", 0);
        obj_setInt(self, (char*)"_hasRootPoint", 0);
        obj_setInt(self, (char*)"_hasCoordinates", 0);
    }

    int _MatrixOS_InputCluster_InputCluster_ClusterId(PikaObj *self) {
        return obj_getInt(self, (char*)"_clusterId");
    }

    char* _MatrixOS_InputCluster_InputCluster_Name(PikaObj *self) {
        return obj_getStr(self, (char*)"_name");
    }

    int _MatrixOS_InputCluster_InputCluster_InputClass(PikaObj *self) {
        return obj_getInt(self, (char*)"_inputClass");
    }

    int _MatrixOS_InputCluster_InputCluster_Shape(PikaObj *self) {
        return obj_getInt(self, (char*)"_shape");
    }

    PikaObj* _MatrixOS_InputCluster_InputCluster_RootPoint(PikaObj *self) {
        PikaObj* pt = newNormalObj(New__MatrixOS_Point_Point);
        Point p(obj_getInt(self, (char*)"_rootPointX"),
                obj_getInt(self, (char*)"_rootPointY"));
        copyCppValueIntoPikaObj<Point>(pt, p);
        return pt;
    }

    PikaObj* _MatrixOS_InputCluster_InputCluster_Dimension(PikaObj *self) {
        PikaObj* dim = newNormalObj(New__MatrixOS_Dimension_Dimension);
        ::Dimension d(obj_getInt(self, (char*)"_dimensionX"),
                      obj_getInt(self, (char*)"_dimensionY"));
        copyCppValueIntoPikaObj<::Dimension>(dim, d);
        return dim;
    }

    int _MatrixOS_InputCluster_InputCluster_InputCount(PikaObj *self) {
        return obj_getInt(self, (char*)"_inputCount");
    }

    pika_bool _MatrixOS_InputCluster_InputCluster_HasRootPoint(PikaObj *self) {
        return obj_getInt(self, (char*)"_hasRootPoint") != 0;
    }

    pika_bool _MatrixOS_InputCluster_InputCluster_HasCoordinates(PikaObj *self) {
        return obj_getInt(self, (char*)"_hasCoordinates") != 0;
    }
}
