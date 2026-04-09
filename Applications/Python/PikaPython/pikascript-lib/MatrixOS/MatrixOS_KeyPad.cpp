#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObjUtils.h"

// Same struct as in MatrixOS_KeyEvent.cpp — Python "KeyEvent" backed by KeypadInfo
struct PythonKeyEvent {
  uint16_t id;
  KeypadInfo info;
};

extern "C" {
    // Forward declarations for constructors
    PikaObj* New__MatrixOS_KeyEvent_KeyEvent(Args *args);
    PikaObj* New__MatrixOS_KeyInfo_KeyInfo(Args *args);
    PikaObj* New__MatrixOS_Point_Point(Args *args);
    void _MatrixOS_Point_Point___init__(PikaObj *self, int x, int y);

    // KeyPad class implementation — polls from MatrixOS::Input
    Arg* _MatrixOS_KeyPad_Get(PikaObj *self, int timeout_ms) {
        InputEvent inputEvent;
        bool success = MatrixOS::Input::Get(&inputEvent, timeout_ms);

        if (success) {
            PythonKeyEvent evt;
            evt.id = Device::KeyPad::InputIdToLegacyKeyId(inputEvent.id);
            evt.info = inputEvent.keypad;

            PikaObj* event_obj = newNormalObj(New__MatrixOS_KeyEvent_KeyEvent);
            copyCppObjIntoPikaObj<PythonKeyEvent>(event_obj, evt);
            return arg_newObj(event_obj);
        }

        return arg_newNone();
    }

    Arg* _MatrixOS_KeyPad_GetKey(PikaObj *self, PikaObj* keyXY) {
        Point* point_ptr = getCppObjPtrInPikaObj<Point>(keyXY);
        if (!point_ptr) return arg_newNone();

        KeypadInfo kpi = MatrixOS::Input::GetKeypadState(*point_ptr);

        PikaObj* info_obj = newNormalObj(New__MatrixOS_KeyInfo_KeyInfo);
        copyCppObjIntoPikaObj<KeypadInfo>(info_obj, kpi);
        return arg_newObj(info_obj);
    }

    Arg* _MatrixOS_KeyPad_GetKeyByID(PikaObj *self, int keyID) {
        InputId id = Device::KeyPad::BridgeKeyId((uint16_t)keyID);
        InputSnapshot snap;
        if (!MatrixOS::Input::GetState(id, &snap))
            return arg_newNone();

        PikaObj* info_obj = newNormalObj(New__MatrixOS_KeyInfo_KeyInfo);
        copyCppObjIntoPikaObj<KeypadInfo>(info_obj, snap.keypad);
        return arg_newObj(info_obj);
    }

    void _MatrixOS_KeyPad_Clear(PikaObj *self) {
        MatrixOS::Input::ClearState();
    }

    int _MatrixOS_KeyPad_XY2ID(PikaObj *self, PikaObj* xy) {
        Point* point_ptr = getCppObjPtrInPikaObj<Point>(xy);
        if (!point_ptr) return -1;

        vector<InputId> ids;
        MatrixOS::Input::GetInputsAt(*point_ptr, &ids);
        if (ids.empty()) return UINT16_MAX;
        return Device::KeyPad::InputIdToLegacyKeyId(ids[0]);
    }

    Arg* _MatrixOS_KeyPad_ID2XY(PikaObj *self, int keyID) {
        InputId id = Device::KeyPad::BridgeKeyId((uint16_t)keyID);
        Point point;
        if (!MatrixOS::Input::TryGetPoint(id, &point)) {
            return arg_newNone();
        }

        PikaObj* xy = newNormalObj(New__MatrixOS_Point_Point);
        _MatrixOS_Point_Point___init__(xy, point.x, point.y);

        return arg_newObj(xy);
    }
}