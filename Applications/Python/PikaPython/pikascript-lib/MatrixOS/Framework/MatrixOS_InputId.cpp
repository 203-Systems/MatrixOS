#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    PikaObj* New__MatrixOS_InputId_InputId(Args *args);

    // InputId constructor — default creates an invalid InputId
    void _MatrixOS_InputId_InputId___init__(PikaObj *self) {
        createCppValueInPikaObj<InputId>(self);
    }

    // Property getters
    int _MatrixOS_InputId_InputId_ClusterId(PikaObj *self) {
        InputId* id = getCppValuePtrInPikaObj<InputId>(self);
        if (!id) return 0;
        return id->clusterId;
    }

    int _MatrixOS_InputId_InputId_MemberId(PikaObj *self) {
        InputId* id = getCppValuePtrInPikaObj<InputId>(self);
        if (!id) return 0;
        return id->memberId;
    }

    // Operators
    pika_bool _MatrixOS_InputId_InputId___eq__(PikaObj *self, PikaObj* other) {
        InputId* a = getCppValuePtrInPikaObj<InputId>(self);
        InputId* b = getCppValuePtrInPikaObj<InputId>(other);
        if (!a || !b) return false;
        return *a == *b;
    }

    pika_bool _MatrixOS_InputId_InputId___ne__(PikaObj *self, PikaObj* other) {
        return !_MatrixOS_InputId_InputId___eq__(self, other);
    }

    pika_bool _MatrixOS_InputId_InputId___bool__(PikaObj *self) {
        InputId* id = getCppValuePtrInPikaObj<InputId>(self);
        if (!id) return false;
        return (bool)(*id);
    }

    // Static factory methods
    PikaObj* _MatrixOS_InputId_InputId_FunctionKey(PikaObj *self) {
        InputId fk = InputId::FunctionKey();
        PikaObj* obj = newNormalObj(New__MatrixOS_InputId_InputId);
        copyCppValueIntoPikaObj<InputId>(obj, fk);
        return obj;
    }

    PikaObj* _MatrixOS_InputId_InputId_Invalid(PikaObj *self) {
        InputId inv = InputId::Invalid();
        PikaObj* obj = newNormalObj(New__MatrixOS_InputId_InputId);
        copyCppValueIntoPikaObj<InputId>(obj, inv);
        return obj;
    }
}
