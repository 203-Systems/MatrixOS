#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    PikaObj* New__MatrixOS_KeypadInfo_KeypadInfo(Args *args);

    void _MatrixOS_KeypadInfo_KeypadInfo___init__(PikaObj *self) {
        createCppValueInPikaObj<KeypadInfo>(self);
    }

    int _MatrixOS_KeypadInfo_KeypadInfo_State(PikaObj *self) {
        KeypadInfo* info = getCppValuePtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0;
        return (int)info->state;
    }

    pika_float _MatrixOS_KeypadInfo_KeypadInfo_Force(PikaObj *self) {
        KeypadInfo* info = getCppValuePtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0.0;
        return (float)info->pressure;
    }

    pika_float _MatrixOS_KeypadInfo_KeypadInfo_Value(PikaObj *self, int index) {
        KeypadInfo* info = getCppValuePtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0.0;
        if (index == 0) return (float)info->pressure;
        if (index == 1) return (float)info->velocity;
        return 0.0;
    }

    int _MatrixOS_KeypadInfo_KeypadInfo_LastEventTime(PikaObj *self) {
        KeypadInfo* info = getCppValuePtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0;
        return info->lastEventTime;
    }

    pika_bool _MatrixOS_KeypadInfo_KeypadInfo_Hold(PikaObj *self) {
        KeypadInfo* info = getCppValuePtrInPikaObj<KeypadInfo>(self);
        if (!info) return false;
        return info->Hold();
    }

    pika_bool _MatrixOS_KeypadInfo_KeypadInfo_Active(PikaObj *self) {
        KeypadInfo* info = getCppValuePtrInPikaObj<KeypadInfo>(self);
        if (!info) return false;
        return info->Active();
    }

    int _MatrixOS_KeypadInfo_KeypadInfo_HoldTime(PikaObj *self) {
        KeypadInfo* info = getCppValuePtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0;
        if (!info->Active()) return 0;
        return (int)(MatrixOS::SYS::Millis() - info->lastEventTime);
    }

    pika_bool _MatrixOS_KeypadInfo_KeypadInfo___bool__(PikaObj *self) {
        KeypadInfo* info = getCppValuePtrInPikaObj<KeypadInfo>(self);
        if (!info) return false;
        return info->state >= KeypadState::Pressed && info->state <= KeypadState::Released;
    }
}
