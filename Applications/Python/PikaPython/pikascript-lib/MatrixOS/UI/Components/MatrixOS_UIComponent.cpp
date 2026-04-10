#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "UI/Component/UIComponent.h"
#include "UI/UI.h"
#include "../../PikaObjUtils.h"
#include "../../PikaCallbackUtils.h"

extern "C" {
    // UIComponent constructor — heap-allocated via handle wrapper
    void _MatrixOS_UIComponent_UIComponent___init__(PikaObj *self) {
        UIComponent* comp = createCppHandleInPikaObj<UIComponent>(self);
        obj_setPtr(self, (char*)"_component", static_cast<UIComponent*>(comp));
        InitCallbackContext(self);
    }

    // Close method — deterministic teardown.
    // Returns false if the native object is still referenced by a live UI.
    pika_bool _MatrixOS_UIComponent_UIComponent_Close(PikaObj *self) {
        UIComponent* comp = getCppHandlePtrInPikaObj<UIComponent>(self);
        if (comp && UI::IsComponentAttached(comp)) {
            return false;
        }

        InvalidateCallbackContext(self);
        destroyCppHandleInPikaObj<UIComponent>(self);
        obj_setPtr(self, (char*)"_component", nullptr);
        ClearCallbackInPikaObj(self, (char*)"enableFunc");
        DestroyCallbackContext(self);
        return true;
    }

    // SetEnabled method
    pika_bool _MatrixOS_UIComponent_UIComponent_SetEnabled(PikaObj *self, pika_bool enabled) {
        UIComponent* component = getCppHandlePtrInPikaObj<UIComponent>(self);
        if (!component) return false;

        component->SetEnabled(enabled);
        return true;
    }

    // SetEnableFunc method
    pika_bool _MatrixOS_UIComponent_UIComponent_SetEnableFunc(PikaObj *self, Arg* enableFunc) {
        UIComponent* component = getCppHandlePtrInPikaObj<UIComponent>(self);
        if (!component) return false;

        SaveCallbackObjToPikaObj(self, (char*)"enableFunc", enableFunc);
        PythonCallbackContext* ctx = GetCallbackContext(self);

        component->SetEnableFunc([ctx]() -> bool {
            Arg* result = SafeCallCallback0(ctx, (char*)"enableFunc");

            bool retval = false;
            if (result) {
                if(arg_getType(result) == ARG_TYPE_BOOL)
                {
                    retval = arg_getBool(result);
                }
                arg_deinit(result);
            }

            return retval;
        });

        return true;
    }
}