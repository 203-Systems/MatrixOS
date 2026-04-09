#include <functional>
#include "MatrixOS.h"
#include "UI/Component/UIButton.h"
#include "UI/UI.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../../PikaObjUtils.h"
#include "../../PikaCallbackUtils.h"

extern "C" {

    // UIButton constructor — heap-allocated via handle wrapper
    void _MatrixOS_UIButton_UIButton___init__(PikaObj *self) {
        UIButton* btn = createCppHandleInPikaObj<UIButton>(self);
        obj_setPtr(self, (char*)"_component", static_cast<UIComponent*>(btn));
    }

    // Close method — deterministic teardown.
    // Refuses to destroy if the native object is still referenced by a live UI.
    void _MatrixOS_UIButton_UIButton_Close(PikaObj *self) {
        UIButton* btn = getCppHandlePtrInPikaObj<UIButton>(self);
        if (btn && UI::IsComponentAttached(static_cast<UIComponent*>(btn))) {
            return;  // still owned by a UI
        }

        destroyCppHandleInPikaObj<UIButton>(self);
        obj_setPtr(self, (char*)"_component", nullptr);
        ClearCallbackInPikaObj(self, (char*)"colorFunc");
        ClearCallbackInPikaObj(self, (char*)"pressCallback");
        ClearCallbackInPikaObj(self, (char*)"holdCallback");
    }

    // SetName method
    pika_bool _MatrixOS_UIButton_UIButton_SetName(PikaObj *self, char* name) {
        UIButton* button = getCppHandlePtrInPikaObj<UIButton>(self);
        if (!button) return false;

        button->SetName(string(name));
        return true;
    }

    // SetColor method
    pika_bool _MatrixOS_UIButton_UIButton_SetColor(PikaObj *self, PikaObj* color) {
        UIButton* button = getCppHandlePtrInPikaObj<UIButton>(self);
        if (!button) return false;

        Color* colorPtr = getCppValuePtrInPikaObj<Color>(color);
        if (!colorPtr) return false;

        button->SetColor(*colorPtr);
        return true;
    }

    // SetColorFunc method
    pika_bool _MatrixOS_UIButton_UIButton_SetColorFunc(PikaObj *self, Arg* colorFunc) {
        UIButton* button = getCppHandlePtrInPikaObj<UIButton>(self);
        if (!button) return false;

        SaveCallbackObjToPikaObj(self, (char*)"colorFunc", colorFunc);

        button->SetColorFunc([self]() -> Color {
            if (!hasCppHandleInPikaObj(self)) return Color::White;
            Color retval = Color::White;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"colorFunc");

            if (result) {
                if (arg_getType(result) == ARG_TYPE_OBJECT) {
                    PikaObj* colorObj = arg_getObj(result);
                    Color* color = getCppValuePtrInPikaObj<Color>(colorObj);
                    if (color) {
                        retval = *color;
                    }
                }
                arg_deinit(result);
            }

            return retval;
        });

        return true;
    }

    // SetSize method
    pika_bool _MatrixOS_UIButton_UIButton_SetSize(PikaObj *self, PikaObj* dimension) {
        UIButton* button = getCppHandlePtrInPikaObj<UIButton>(self);
        if (!button) return false;

        Dimension* dimPtr = getCppValuePtrInPikaObj<Dimension>(dimension);
        if (!dimPtr) return false;

        button->SetSize(*dimPtr);
        return true;
    }

    // OnPress method
    pika_bool _MatrixOS_UIButton_UIButton_OnPress(PikaObj *self, Arg* pressCallback) {
        UIButton* button = getCppHandlePtrInPikaObj<UIButton>(self);
        if (!button) return false;

        SaveCallbackObjToPikaObj(self, (char*)"pressCallback", pressCallback);

        button->OnPress([self]() {
            if (!hasCppHandleInPikaObj(self)) return;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"pressCallback");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // OnHold method
    pika_bool _MatrixOS_UIButton_UIButton_OnHold(PikaObj *self, Arg* holdCallback) {
        UIButton* button = getCppHandlePtrInPikaObj<UIButton>(self);
        if (!button) return false;

        SaveCallbackObjToPikaObj(self, (char*)"holdCallback", holdCallback);

        button->OnHold([self]() {
            if (!hasCppHandleInPikaObj(self)) return;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"holdCallback");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }
}