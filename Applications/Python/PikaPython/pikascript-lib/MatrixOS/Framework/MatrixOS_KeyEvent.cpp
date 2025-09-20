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
        return keyEvent->id;
    }

    Arg* _MatrixOS_KeyEvent_KeyEvent_KeyInfo(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return arg_newNone();
        
        // Create a new KeyInfo Python object
        PikaObj* keyInfoObj = New__MatrixOS_KeyInfo_KeyInfo(NULL);
        
        // Copy the KeyInfo data from the KeyEvent
        copyCppObjIntoPikaObj<KeyInfo>(keyInfoObj, keyEvent->info);
        
        return arg_newObj(keyInfoObj);
    }
}