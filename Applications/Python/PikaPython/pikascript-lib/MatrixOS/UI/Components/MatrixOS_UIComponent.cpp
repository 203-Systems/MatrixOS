#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include <functional>
#include "UI/Component/UIComponent.h"
#include "../../PikaObjUtils.h"
#include "../../PikaCallbackUtils.h"

extern "C" {
    // UIComponent constructor
    void _MatrixOS_UIComponent_UIComponent___init__(PikaObj *self) {
        createCppObjPtrInPikaObj<UIComponent>(self);
    }

    // SetEnabled method
    pika_bool _MatrixOS_UIComponent_UIComponent_SetEnabled(PikaObj *self, pika_bool enabled) {
        UIComponent* component = getCppObjPtrInPikaObj<UIComponent>(self);
        if (!component) return false;

        component->SetEnabled(enabled);
        return true;
    }

    // SetEnableFunc method
    pika_bool _MatrixOS_UIComponent_UIComponent_SetEnableFunc(PikaObj *self, Arg* enableFunc) {
        UIComponent* component = getCppObjPtrInPikaObj<UIComponent>(self);
        if (!component) return false;
        
        SaveCallbackObjToPikaObj(self, (char*)"enableFunc", enableFunc);

        component->SetEnableFunc([self]() -> bool {
            Arg* result = CallCallbackInPikaObj0(self, (char*)"enableFunc");

            // Convert result to bool
            bool retval = false;
            if (result) {
                if(arg_getType(result) == ARG_TYPE_BOOL)
                {
                    retval = arg_getBool(result);
                }
                else
                {
                    // Throw error
                }
                arg_deinit(result);
            }

            return retval;
        });

        return true;
    }
}