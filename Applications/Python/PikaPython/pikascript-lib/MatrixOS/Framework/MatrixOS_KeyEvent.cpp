#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    // PikaObj constructor
    PikaObj* New__MatrixOS_KeyInfo_KeyInfo(Args *args);

    // constructor
    void _MatrixOS_KeyEvent_KeyEvent___init__(PikaObj *self) {
        createCppObjPtrInPikaObj<KeyEvent>(self);
    }

    // Getters
    int _MatrixOS_KeyEvent_KeyEvent_ID(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return 0;
        return keyEvent->ID();
    }

    // Pass-through methods to KeyInfo
    int _MatrixOS_KeyEvent_KeyEvent_State(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return 0;
        return (int)keyEvent->State();
    }

    pika_bool _MatrixOS_KeyEvent_KeyEvent_Hold(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return false;
        return keyEvent->Hold();
    }

    int _MatrixOS_KeyEvent_KeyEvent_HoldTime(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return 0;
        return keyEvent->HoldTime();
    }

    pika_bool _MatrixOS_KeyEvent_KeyEvent_Active(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return false;
        return keyEvent->Active();
    }

    pika_float _MatrixOS_KeyEvent_KeyEvent_Force(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return 0.0;
        return (float)keyEvent->Force();
    }

    pika_float _MatrixOS_KeyEvent_KeyEvent_Value(PikaObj *self, int index) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return 0.0;
        return (float)keyEvent->Value(index);
    }

    // Boolean conversion operator
    pika_bool _MatrixOS_KeyEvent_KeyEvent___bool__(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return false;
        return (*keyEvent);
    }

    Arg* _MatrixOS_KeyEvent_KeyEvent_KeyInfo(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return arg_newNone();
        
        // Create a new KeyInfo Python object
        PikaObj* keyInfoObj = newNormalObj(New__MatrixOS_KeyInfo_KeyInfo);
        
        // Copy the KeyInfo data from the KeyEvent
        copyCppObjIntoPikaObj<KeyInfo>(keyInfoObj, keyEvent->info);
        
        return arg_newObj(keyInfoObj);
    }
}