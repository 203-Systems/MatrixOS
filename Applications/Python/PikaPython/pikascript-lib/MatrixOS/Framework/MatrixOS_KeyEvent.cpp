#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

// Python-side "KeyEvent" backed by new input types.
// Keeps the same Python API surface but no longer depends on the C++ KeyEvent struct.
struct PythonKeyEvent {
  uint16_t id;
  KeypadInfo info;
};

extern "C" {
    // PikaObj constructor
    PikaObj* New__MatrixOS_KeyInfo_KeyInfo(Args *args);

    // constructor
    void _MatrixOS_KeyEvent_KeyEvent___init__(PikaObj *self) {
        createCppObjPtrInPikaObj<PythonKeyEvent>(self);
    }

    // Getters
    int _MatrixOS_KeyEvent_KeyEvent_ID(PikaObj *self) {
        PythonKeyEvent* evt = getCppObjPtrInPikaObj<PythonKeyEvent>(self);
        if (!evt) return 0;
        return evt->id;
    }

    // Pass-through methods to KeypadInfo
    int _MatrixOS_KeyEvent_KeyEvent_State(PikaObj *self) {
        PythonKeyEvent* evt = getCppObjPtrInPikaObj<PythonKeyEvent>(self);
        if (!evt) return 0;
        return (int)evt->info.state;
    }

    pika_bool _MatrixOS_KeyEvent_KeyEvent_Hold(PikaObj *self) {
        PythonKeyEvent* evt = getCppObjPtrInPikaObj<PythonKeyEvent>(self);
        if (!evt) return false;
        return evt->info.Hold();
    }

    int _MatrixOS_KeyEvent_KeyEvent_HoldTime(PikaObj *self) {
        PythonKeyEvent* evt = getCppObjPtrInPikaObj<PythonKeyEvent>(self);
        if (!evt) return 0;
        if (!evt->info.Active()) return 0;
        return (int)(MatrixOS::SYS::Millis() - evt->info.lastEventTime);
    }

    pika_bool _MatrixOS_KeyEvent_KeyEvent_Active(PikaObj *self) {
        PythonKeyEvent* evt = getCppObjPtrInPikaObj<PythonKeyEvent>(self);
        if (!evt) return false;
        return evt->info.Active();
    }

    pika_float _MatrixOS_KeyEvent_KeyEvent_Force(PikaObj *self) {
        PythonKeyEvent* evt = getCppObjPtrInPikaObj<PythonKeyEvent>(self);
        if (!evt) return 0.0;
        return (float)evt->info.pressure;
    }

    pika_float _MatrixOS_KeyEvent_KeyEvent_Value(PikaObj *self, int index) {
        PythonKeyEvent* evt = getCppObjPtrInPikaObj<PythonKeyEvent>(self);
        if (!evt) return 0.0;
        if (index == 0) return (float)evt->info.pressure;
        if (index == 1) return (float)evt->info.velocity;
        return 0.0;
    }

    // Boolean conversion operator
    pika_bool _MatrixOS_KeyEvent_KeyEvent___bool__(PikaObj *self) {
        PythonKeyEvent* evt = getCppObjPtrInPikaObj<PythonKeyEvent>(self);
        if (!evt) return false;
        return evt->info.state >= KeypadState::Pressed && evt->info.state <= KeypadState::Released;
    }

    Arg* _MatrixOS_KeyEvent_KeyEvent_KeyInfo(PikaObj *self) {
        PythonKeyEvent* evt = getCppObjPtrInPikaObj<PythonKeyEvent>(self);
        if (!evt) return arg_newNone();

        PikaObj* keyInfoObj = newNormalObj(New__MatrixOS_KeyInfo_KeyInfo);
        copyCppObjIntoPikaObj<KeypadInfo>(keyInfoObj, evt->info);

        return arg_newObj(keyInfoObj);
    }
}