#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    // PikaObj constructor
    PikaObj* New__MatrixOS_KeyInfo_KeyInfo(Args *args);

    // constructor
    void _MatrixOS_KeyInfo_KeyInfo___init__(PikaObj *self) {
        createCppObjPtrInPikaObj<KeyInfo>(self);
    }

    // Getter methods
    int _MatrixOS_KeyInfo_KeyInfo_State(PikaObj *self) {
        KeyInfo* keyInfo = getCppObjPtrInPikaObj<KeyInfo>(self);
        if (!keyInfo) return 0;
        return (int)keyInfo->state;
    }

    pika_float _MatrixOS_KeyInfo_KeyInfo_Velocity(PikaObj *self) {
        KeyInfo* keyInfo = getCppObjPtrInPikaObj<KeyInfo>(self);
        if (!keyInfo) return 0.0;
        return (float)keyInfo->velocity;
    }

    int _MatrixOS_KeyInfo_KeyInfo_LastEventTime(PikaObj *self) {
        KeyInfo* keyInfo = getCppObjPtrInPikaObj<KeyInfo>(self);
        if (!keyInfo) return 0;
        return keyInfo->lastEventTime;
    }

    pika_bool _MatrixOS_KeyInfo_KeyInfo_Hold(PikaObj *self) {
        KeyInfo* keyInfo = getCppObjPtrInPikaObj<KeyInfo>(self);
        if (!keyInfo) return false;
        return keyInfo->hold;
    }

    // Methods
    pika_bool _MatrixOS_KeyInfo_KeyInfo_Active(PikaObj *self) {
        KeyInfo* keyInfo = getCppObjPtrInPikaObj<KeyInfo>(self);
        if (!keyInfo) return false;
        return keyInfo->Active();
    }

    int _MatrixOS_KeyInfo_KeyInfo_HoldTime(PikaObj *self) {
        KeyInfo* keyInfo = getCppObjPtrInPikaObj<KeyInfo>(self);
        if (!keyInfo) return 0;
        return keyInfo->HoldTime();
    }

    // Boolean conversion operator
    pika_bool _MatrixOS_KeyInfo_KeyInfo___bool__(PikaObj *self) {
        KeyInfo* keyInfo = getCppObjPtrInPikaObj<KeyInfo>(self);
        if (!keyInfo) return false;
        return (*keyInfo);
    }
}