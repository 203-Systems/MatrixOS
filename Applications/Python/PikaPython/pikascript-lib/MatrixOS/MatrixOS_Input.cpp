#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "PikaObjUtils.h"

// C++ helper (defined in Framework/MatrixOS_InputCluster.cpp)
void populateInputClusterInPikaObj(PikaObj* obj, const InputCluster& c);

extern "C" {
    PikaObj* New__MatrixOS_InputId_InputId(Args *args);
    PikaObj* New__MatrixOS_InputEvent_InputEvent(Args *args);
    PikaObj* New__MatrixOS_KeypadInfo_KeypadInfo(Args *args);
    PikaObj* New__MatrixOS_InputCluster_InputCluster(Args *args);
    void _MatrixOS_InputCluster_InputCluster___init__(PikaObj *self);
    PikaObj* New__MatrixOS_Point_Point(Args *args);
    void _MatrixOS_Point_Point___init__(PikaObj *self, int x, int y);
    PikaObj* New_PikaStdData_List(Args *args);
    void PikaStdData_List___init__(PikaObj* self);
    void PikaStdData_List_append(PikaObj* self, Arg* arg);

    // GetEvent — polls for the next input event.
    // Returns InputEvent on success, None on timeout.
    Arg* _MatrixOS_Input_GetEvent(PikaObj *self, int timeout_ms) {
        InputEvent event;
        if (MatrixOS::Input::Get(&event, (uint32_t)timeout_ms)) {
            PikaObj* eventObj = newNormalObj(New__MatrixOS_InputEvent_InputEvent);
            copyCppValueIntoPikaObj<InputEvent>(eventObj, event);
            return arg_newObj(eventObj);
        }
        return arg_newNone();
    }

    // GetState — gets the current snapshot of an input.
    // Returns KeypadInfo for keypad inputs, None for non-keypad or not found.
    Arg* _MatrixOS_Input_GetState(PikaObj *self, PikaObj* input_id) {
        InputId* id = getCppValuePtrInPikaObj<InputId>(input_id);
        if (!id) return arg_newNone();

        InputSnapshot snap;
        if (!MatrixOS::Input::GetState(*id, &snap)) {
            return arg_newNone();
        }

        // Only keypad-class snapshots can be safely read as KeypadInfo
        if (snap.inputClass != ::InputClass::Keypad) {
            return arg_newNone();
        }

        PikaObj* infoObj = newNormalObj(New__MatrixOS_KeypadInfo_KeypadInfo);
        copyCppValueIntoPikaObj<KeypadInfo>(infoObj, snap.keypad);
        return arg_newObj(infoObj);
    }

    // GetPosition — get the grid coordinate of an input.
    // Returns Point on success, None if input has no position.
    Arg* _MatrixOS_Input_GetPosition(PikaObj *self, PikaObj* input_id) {
        InputId* id = getCppValuePtrInPikaObj<InputId>(input_id);
        if (!id) return arg_newNone();

        Point point;
        if (!MatrixOS::Input::GetPosition(*id, &point))
            return arg_newNone();

        PikaObj* ptObj = newNormalObj(New__MatrixOS_Point_Point);
        _MatrixOS_Point_Point___init__(ptObj, point.x, point.y);
        return arg_newObj(ptObj);
    }

    // GetInputsAt — find all inputs at a grid coordinate.
    // Returns a list of InputId objects.
    PikaObj* _MatrixOS_Input_GetInputsAt(PikaObj *self, PikaObj* point) {
        Point* pt = getCppValuePtrInPikaObj<Point>(point);

        PikaObj* list = newNormalObj(New_PikaStdData_List);
        PikaStdData_List___init__(list);

        if (!pt) return list;

        vector<InputId> ids;
        MatrixOS::Input::GetInputsAt(*pt, &ids);

        for (const auto& id : ids) {
            PikaObj* idObj = newNormalObj(New__MatrixOS_InputId_InputId);
            copyCppValueIntoPikaObj<InputId>(idObj, id);
            PikaStdData_List_append(list, arg_newObj(idObj));
        }
        return list;
    }

    // ClearQueue — discard all pending input events.
    void _MatrixOS_Input_ClearQueue(PikaObj *self) {
        MatrixOS::Input::ClearQueue();
    }

    // ClearState — reset all input state (pressure, hold, etc.).
    void _MatrixOS_Input_ClearState(PikaObj *self) {
        MatrixOS::Input::ClearState();
    }

    // FunctionKey — returns the InputId for the function key.
    PikaObj* _MatrixOS_Input_FunctionKey(PikaObj *self) {
        InputId fk = InputId::FunctionKey();
        PikaObj* obj = newNormalObj(New__MatrixOS_InputId_InputId);
        copyCppValueIntoPikaObj<InputId>(obj, fk);
        return obj;
    }

    // GetClusters — returns list of InputCluster objects.
    PikaObj* _MatrixOS_Input_GetClusters(PikaObj *self) {
        const auto& clusters = MatrixOS::Input::GetClusters();

        PikaObj* list = newNormalObj(New_PikaStdData_List);
        PikaStdData_List___init__(list);

        for (const auto& c : clusters) {
            PikaObj* clusterObj = newNormalObj(New__MatrixOS_InputCluster_InputCluster);
            _MatrixOS_InputCluster_InputCluster___init__(clusterObj);
            populateInputClusterInPikaObj(clusterObj, c);
            PikaStdData_List_append(list, arg_newObj(clusterObj));
        }
        return list;
    }

    // GetPrimaryGridCluster — returns InputCluster or None.
    Arg* _MatrixOS_Input_GetPrimaryGridCluster(PikaObj *self) {
        const InputCluster* cluster = MatrixOS::Input::GetPrimaryGridCluster();
        if (!cluster) return arg_newNone();

        PikaObj* clusterObj = newNormalObj(New__MatrixOS_InputCluster_InputCluster);
        _MatrixOS_InputCluster_InputCluster___init__(clusterObj);
        populateInputClusterInPikaObj(clusterObj, *cluster);
        return arg_newObj(clusterObj);
    }
}
