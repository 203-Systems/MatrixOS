#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    // PikaObj constructor
    PikaObj* New__MatrixOS_KeyEvent_KeyEvent(Args *args);
    PikaObj* New__MatrixOS_KeyInfo_KeyInfo(Args *args);

    // KeyEvent constructor
    void _MatrixOS_KeyEvent_KeyEvent___init__(PikaObj *self, int id, PikaObj* info) {
        KeyInfo* keyInfo = getCppObjPtrInPikaObj<KeyInfo>(info);
        if (!keyInfo) return;

        KeyEvent event = {
            .id = (uint16_t)id,
            .info = *keyInfo
        };
        createCppObjPtrInPikaObj<KeyEvent>(self, event);
    }

    void _MatrixOS_KeyEvent_KeyEvent___del__(PikaObj *self) {
        deleteCppObjInPikaObj<KeyEvent>(self);
    }

    // Getters
    int _MatrixOS_KeyEvent_KeyEvent_ID(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return 0;
        return keyEvent->id;
    }

    PikaObj* _MatrixOS_KeyEvent_KeyEvent_KeyInfo(PikaObj *self) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return nullptr;
        
        // Create a new KeyInfo Python object
        PikaObj* keyInfoObj = New__MatrixOS_KeyInfo_KeyInfo(NULL);
        
        // Copy the KeyInfo data from the KeyEvent
        copyCppObjIntoPikaObj<KeyInfo>(keyInfoObj, keyEvent->info);
        
        return keyInfoObj;
    }

    // Setters
    void _MatrixOS_KeyEvent_KeyEvent_SetID(PikaObj *self, int id) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        if (!keyEvent) return;
        keyEvent->id = (uint16_t)id;
    }

    void _MatrixOS_KeyEvent_KeyEvent_SetInfo(PikaObj *self, PikaObj* info) {
        KeyEvent* keyEvent = getCppObjPtrInPikaObj<KeyEvent>(self);
        KeyInfo* keyInfo = getCppObjPtrInPikaObj<KeyInfo>(info);
        
        if (!keyEvent || !keyInfo) return;
        
        // Copy the KeyInfo data
        keyEvent->info = *keyInfo;
    }
}