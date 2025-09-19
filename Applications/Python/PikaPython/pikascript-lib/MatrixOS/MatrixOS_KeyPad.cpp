#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObjUtils.h"

extern "C" {
    // Forward declarations for constructors
    PikaObj* New__MatrixOS_KeyEvent_KeyEvent(Args *args);
    PikaObj* New__MatrixOS_KeyInfo_KeyInfo(Args *args);
    PikaObj* New__MatrixOS_Point_Point(Args *args);
    void _MatrixOS_Point_Point___init__(PikaObj *self, int x, int y);

    // KeyPad class implementation
    PikaObj* _MatrixOS_KeyPad_Get(PikaObj *self, int timeout_ms) {
        KeyEvent event;
        bool success = MatrixOS::KeyPad::Get(&event, timeout_ms);

        if (success) {
            // Create new KeyEvent object and copy the C++ object into it
            PikaObj* key_event = New__MatrixOS_KeyEvent_KeyEvent(NULL);
            copyCppObjIntoPikaObj<KeyEvent>(key_event, event);
            return key_event;
        } else {
            // Return None on timeout/failure
            return nullptr;
        }
    }

    PikaObj* _MatrixOS_KeyPad_GetKey(PikaObj *self, PikaObj* keyXY) {
        // Get Point object from PikaObj
        Point* point_ptr = getCppObjPtrInPikaObj<Point>(keyXY);
        if (!point_ptr) return nullptr;

        KeyInfo* info = MatrixOS::KeyPad::GetKey(*point_ptr);

        if (info != nullptr) {
            PikaObj* key_info = New__MatrixOS_KeyInfo_KeyInfo(NULL);
            copyCppObjIntoPikaObj<KeyInfo>(key_info, *info);
            return key_info;
        } else {
            // Return None if invalid position
            return nullptr;
        }
    }

    PikaObj* _MatrixOS_KeyPad_GetKeyByID(PikaObj *self, int keyID) {
        KeyInfo* info = MatrixOS::KeyPad::GetKey(keyID);

        if (info != nullptr) {
            PikaObj* key_info = New__MatrixOS_KeyInfo_KeyInfo(NULL);
            copyCppObjIntoPikaObj<KeyInfo>(key_info, *info);
            return key_info;
        } else {
            // Return None if invalid ID
            return nullptr;
        }
    }

    void _MatrixOS_KeyPad_Clear(PikaObj *self) {
        MatrixOS::KeyPad::Clear();
    }

    int _MatrixOS_KeyPad_XY2ID(PikaObj *self, PikaObj* xy) {
        // Get Point object from PikaObj
        Point* point_ptr = getCppObjPtrInPikaObj<Point>(xy);
        if (!point_ptr) return -1;  // Return invalid ID

        return MatrixOS::KeyPad::XY2ID(*point_ptr);
    }

    PikaObj* _MatrixOS_KeyPad_ID2XY(PikaObj *self, int keyID) {
        Point point = MatrixOS::KeyPad::ID2XY(keyID);

        // Check if the returned point is valid (ID2XY returns Point::Invalid() for invalid IDs)
        if (point == Point::Invalid()) {
            // Return None if invalid ID
            return nullptr;
        }

        PikaObj* xy = New__MatrixOS_Point_Point(NULL);
        _MatrixOS_Point_Point___init__(xy, point.x, point.y);

        return xy;
    }
}