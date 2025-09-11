#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"

extern "C" {
    // KeyPad class implementation
    PikaObj* _MatrixOS_KeyPad_Get(PikaObj *self, int timeout_ms) {
        KeyEvent event;
        bool success = MatrixOS::KeyPad::Get(&event, timeout_ms);
        
        PikaObj* key_event = newNormalObj(New_PikaObj);
        if (success) {
            obj_setInt(key_event, (char*)"id", event.id);
            obj_setInt(key_event, (char*)"state", event.info.state);
            obj_setInt(key_event, (char*)"velocity", (int)event.info.velocity);
            obj_setBool(key_event, (char*)"hold", event.info.hold);
            obj_setInt(key_event, (char*)"lastEventTime", event.info.lastEventTime);
        } else {
            // Return invalid event
            obj_setInt(key_event, (char*)"id", UINT16_MAX);
            obj_setInt(key_event, (char*)"state", IDLE);
            obj_setInt(key_event, (char*)"velocity", 0);
            obj_setBool(key_event, (char*)"hold", false);
            obj_setInt(key_event, (char*)"lastEventTime", 0);
        }
        
        return key_event;
    }

    PikaObj* _MatrixOS_KeyPad_GetKey(PikaObj *self, PikaObj* keyXY) {
        int x = obj_getInt(keyXY, (char*)"x");
        int y = obj_getInt(keyXY, (char*)"y");
        Point point(x, y);
        
        KeyInfo* info = MatrixOS::KeyPad::GetKey(point);
        
        PikaObj* key_info = newNormalObj(New_PikaObj);
        if (info != nullptr) {
            obj_setInt(key_info, (char*)"state", info->state);
            obj_setInt(key_info, (char*)"velocity", (int)info->velocity);
            obj_setBool(key_info, (char*)"hold", info->hold);
            obj_setInt(key_info, (char*)"lastEventTime", info->lastEventTime);
        } else {
            obj_setInt(key_info, (char*)"state", IDLE);
            obj_setInt(key_info, (char*)"velocity", 0);
            obj_setBool(key_info, (char*)"hold", false);
            obj_setInt(key_info, (char*)"lastEventTime", 0);
        }
        
        return key_info;
    }

    PikaObj* _MatrixOS_KeyPad_GetKeyByID(PikaObj *self, int keyID) {
        KeyInfo* info = MatrixOS::KeyPad::GetKey(keyID);
        
        PikaObj* key_info = newNormalObj(New_PikaObj);
        if (info != nullptr) {
            obj_setInt(key_info, (char*)"state", info->state);
            obj_setInt(key_info, (char*)"velocity", (int)info->velocity);
            obj_setBool(key_info, (char*)"hold", info->hold);
            obj_setInt(key_info, (char*)"lastEventTime", info->lastEventTime);
        } else {
            obj_setInt(key_info, (char*)"state", IDLE);
            obj_setInt(key_info, (char*)"velocity", 0);
            obj_setBool(key_info, (char*)"hold", false);
            obj_setInt(key_info, (char*)"lastEventTime", 0);
        }
        
        return key_info;
    }

    void _MatrixOS_KeyPad_Clear(PikaObj *self) {
        MatrixOS::KeyPad::Clear();
    }

    int _MatrixOS_KeyPad_XY2ID(PikaObj *self, PikaObj* xy) {
        int x = obj_getInt(xy, (char*)"x");
        int y = obj_getInt(xy, (char*)"y");
        Point point(x, y);
        
        return MatrixOS::KeyPad::XY2ID(point);
    }

    PikaObj* _MatrixOS_KeyPad_ID2XY(PikaObj *self, int keyID) {
        Point point = MatrixOS::KeyPad::ID2XY(keyID);
        
        PikaObj* xy = newNormalObj(New_PikaObj);
        obj_setInt(xy, (char*)"x", point.x);
        obj_setInt(xy, (char*)"y", point.y);
        
        return xy;
    }
}