#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    void _MatrixOS_KeyPad_Clear(PikaObj *self);
    PikaObj* _MatrixOS_KeyPad_Get(PikaObj *self, int timeout_ms);
    PikaObj* _MatrixOS_KeyPad_GetKey(PikaObj *self, PikaObj* keyXY);
    PikaObj* _MatrixOS_KeyPad_GetKeyByID(PikaObj *self, int keyID);
    PikaObj* _MatrixOS_KeyPad_ID2XY(PikaObj *self, int keyID);
    int _MatrixOS_KeyPad_XY2ID(PikaObj *self, PikaObj* xy);
}