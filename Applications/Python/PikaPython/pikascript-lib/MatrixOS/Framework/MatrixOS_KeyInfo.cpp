#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    // PikaObj constructor
    PikaObj* New__MatrixOS_KeyInfo_KeyInfo(Args *args);

    // constructor
    void _MatrixOS_KeyInfo_KeyInfo___init__(PikaObj *self) {
        createCppObjPtrInPikaObj<KeypadInfo>(self);
    }

    // Getter methods
    int _MatrixOS_KeyInfo_KeyInfo_State(PikaObj *self) {
        KeypadInfo* info = getCppObjPtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0;
        return (int)info->state;
    }

    pika_float _MatrixOS_KeyInfo_KeyInfo_Force(PikaObj *self) {
        KeypadInfo* info = getCppObjPtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0.0;
        return (float)info->pressure;
    }

    pika_float _MatrixOS_KeyInfo_KeyInfo_Value(PikaObj *self, int index) {
        KeypadInfo* info = getCppObjPtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0.0;
        if (index == 0) return (float)info->pressure;
        if (index == 1) return (float)info->velocity;
        return 0.0;
    }

    int _MatrixOS_KeyInfo_KeyInfo_LastEventTime(PikaObj *self) {
        KeypadInfo* info = getCppObjPtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0;
        return info->lastEventTime;
    }

    pika_bool _MatrixOS_KeyInfo_KeyInfo_Hold(PikaObj *self) {
        KeypadInfo* info = getCppObjPtrInPikaObj<KeypadInfo>(self);
        if (!info) return false;
        return info->Hold();
    }

    // Methods
    pika_bool _MatrixOS_KeyInfo_KeyInfo_Active(PikaObj *self) {
        KeypadInfo* info = getCppObjPtrInPikaObj<KeypadInfo>(self);
        if (!info) return false;
        return info->Active();
    }

    int _MatrixOS_KeyInfo_KeyInfo_HoldTime(PikaObj *self) {
        KeypadInfo* info = getCppObjPtrInPikaObj<KeypadInfo>(self);
        if (!info) return 0;
        if (!info->Active()) return 0;
        return (int)(MatrixOS::SYS::Millis() - info->lastEventTime);
    }

    // Boolean conversion operator
    pika_bool _MatrixOS_KeyInfo_KeyInfo___bool__(PikaObj *self) {
        KeypadInfo* info = getCppObjPtrInPikaObj<KeypadInfo>(self);
        if (!info) return false;
        return info->state >= KeypadState::Pressed && info->state <= KeypadState::Released;
    }
}