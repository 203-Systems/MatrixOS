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

    void _MatrixOS_UIComponent_UIComponent___del__(PikaObj *self) {
        deleteCppObjInPikaObj<UIComponent>(self);
    }

    // SetEnabled method
    void _MatrixOS_UIComponent_UIComponent_SetEnabled(PikaObj *self, pika_bool enabled) {
        UIComponent* component = getCppObjPtrInPikaObj<UIComponent>(self);
        if (!component) return;

        component->SetEnabled(enabled);
    }

    // ShouldEnable method
    void _MatrixOS_UIComponent_UIComponent_ShouldEnable(PikaObj *self, Arg* enable_func) {
        UIComponent* component = getCppObjPtrInPikaObj<UIComponent>(self);
        if (!component) return;

        if (NULL == g_pika_user_listener){
            pika_eventListener_init(&g_pika_user_listener);
        }

        uint32_t eventId = RegisterCallback(enable_func);


        component->ShouldEnable([eventId]() -> bool {
            Arg* returnValue = EventCallback(eventId);

            if(arg_getType(returnValue) != ARG_TYPE_BOOL)
            {
                // Throw error
                return false;
            }

            return arg_getBool(returnValue);
        });
    }
}