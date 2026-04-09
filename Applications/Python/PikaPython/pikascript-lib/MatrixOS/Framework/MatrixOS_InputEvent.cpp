#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    PikaObj* New__MatrixOS_InputId_InputId(Args *args);
    PikaObj* New__MatrixOS_InputEvent_InputEvent(Args *args);
    PikaObj* New__MatrixOS_KeypadInfo_KeypadInfo(Args *args);

    // InputEvent constructor — default creates a zeroed event
    void _MatrixOS_InputEvent_InputEvent___init__(PikaObj *self) {
        createCppValueInPikaObj<InputEvent>(self);
    }

    // Returns the InputId that generated this event
    PikaObj* _MatrixOS_InputEvent_InputEvent_Id(PikaObj *self) {
        InputEvent* evt = getCppValuePtrInPikaObj<InputEvent>(self);
        if (!evt) return nullptr;

        PikaObj* idObj = newNormalObj(New__MatrixOS_InputId_InputId);
        copyCppValueIntoPikaObj<InputId>(idObj, evt->id);
        return idObj;
    }

    // Returns the InputClass enum value as int
    int _MatrixOS_InputEvent_InputEvent_InputClass(PikaObj *self) {
        InputEvent* evt = getCppValuePtrInPikaObj<InputEvent>(self);
        if (!evt) return 0;
        return (int)evt->inputClass;
    }

    // Returns KeypadInfo if this is a keypad event, else None
    Arg* _MatrixOS_InputEvent_InputEvent_Keypad(PikaObj *self) {
        InputEvent* evt = getCppValuePtrInPikaObj<InputEvent>(self);
        if (!evt) return arg_newNone();

        if (evt->inputClass != ::InputClass::Keypad) {
            return arg_newNone();
        }

        PikaObj* infoObj = newNormalObj(New__MatrixOS_KeypadInfo_KeypadInfo);
        copyCppValueIntoPikaObj<KeypadInfo>(infoObj, evt->keypad);
        return arg_newObj(infoObj);
    }

    // Boolean: true if the event has a valid input class
    pika_bool _MatrixOS_InputEvent_InputEvent___bool__(PikaObj *self) {
        InputEvent* evt = getCppValuePtrInPikaObj<InputEvent>(self);
        if (!evt) return false;
        return evt->inputClass != ::InputClass::Unknown;
    }
}
