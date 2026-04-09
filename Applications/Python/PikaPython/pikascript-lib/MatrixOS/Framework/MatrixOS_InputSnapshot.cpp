#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    PikaObj* New__MatrixOS_InputId_InputId(Args *args);
    PikaObj* New__MatrixOS_InputSnapshot_InputSnapshot(Args *args);
    PikaObj* New__MatrixOS_KeypadInfo_KeypadInfo(Args *args);

    // InputSnapshot constructor — default creates a zeroed snapshot
    void _MatrixOS_InputSnapshot_InputSnapshot___init__(PikaObj *self) {
        createCppValueInPikaObj<InputSnapshot>(self);
    }

    // Returns the InputId that this snapshot belongs to
    PikaObj* _MatrixOS_InputSnapshot_InputSnapshot_Id(PikaObj *self) {
        InputSnapshot* snap = getCppValuePtrInPikaObj<InputSnapshot>(self);
        if (!snap) return nullptr;

        PikaObj* idObj = newNormalObj(New__MatrixOS_InputId_InputId);
        copyCppValueIntoPikaObj<InputId>(idObj, snap->id);
        return idObj;
    }

    // Returns the InputClass enum value as int
    int _MatrixOS_InputSnapshot_InputSnapshot_InputClass(PikaObj *self) {
        InputSnapshot* snap = getCppValuePtrInPikaObj<InputSnapshot>(self);
        if (!snap) return 0;
        return (int)snap->inputClass;
    }

    // Returns KeypadInfo if this is a keypad snapshot, else None
    Arg* _MatrixOS_InputSnapshot_InputSnapshot_Keypad(PikaObj *self) {
        InputSnapshot* snap = getCppValuePtrInPikaObj<InputSnapshot>(self);
        if (!snap) return arg_newNone();

        if (snap->inputClass != ::InputClass::Keypad) {
            return arg_newNone();
        }

        PikaObj* infoObj = newNormalObj(New__MatrixOS_KeypadInfo_KeypadInfo);
        copyCppValueIntoPikaObj<KeypadInfo>(infoObj, snap->keypad);
        return arg_newObj(infoObj);
    }

    // Boolean: true if the snapshot has a valid input class
    pika_bool _MatrixOS_InputSnapshot_InputSnapshot___bool__(PikaObj *self) {
        InputSnapshot* snap = getCppValuePtrInPikaObj<InputSnapshot>(self);
        if (!snap) return false;
        return snap->inputClass != ::InputClass::Unknown;
    }
}
