#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include <functional>
#include "UI/Component/UIComponent.h"
#include "UI/UI.h"
#include "../../PikaObjUtils.h"
#include "../../PikaCallbackUtils.h"

extern "C" {
    // UIComponent constructor — heap-allocated via handle wrapper
    void _MatrixOS_UIComponent_UIComponent___init__(PikaObj *self) {
        UIComponent* comp = createCppHandleInPikaObj<UIComponent>(self);
        obj_setPtr(self, (char*)"_component", static_cast<UIComponent*>(comp));
    }

    // Close method — deterministic teardown.
    // Refuses to destroy if the native object is still referenced by a live UI.
    void _MatrixOS_UIComponent_UIComponent_Close(PikaObj *self) {
        UIComponent* comp = getCppHandlePtrInPikaObj<UIComponent>(self);
        if (comp && UI::IsComponentAttached(comp)) {
            return;  // still owned by a UI — caller must ClearUIComponents or Close the UI first
        }

        destroyCppHandleInPikaObj<UIComponent>(self);
        obj_setPtr(self, (char*)"_component", nullptr);
        ClearCallbackInPikaObj(self, (char*)"enableFunc");
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

        component->SetEnableFunc([self]() -> bool {
            if (!hasCppHandleInPikaObj(self)) return false;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"enableFunc");

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