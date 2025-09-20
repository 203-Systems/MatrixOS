#include <functional>
#include "MatrixOS.h"
#include "UI/Component/UIButton.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../../PikaObjUtils.h"
#include "../../PikaCallbackUtils.h"

extern "C" {

    // UIButton constructor
    void _MatrixOS_UIButton_UIButton___init__(PikaObj *self) {
        createCppObjPtrInPikaObj<UIButton>(self);
    }

    // SetName method
    pika_bool _MatrixOS_UIButton_UIButton_SetName(PikaObj *self, char* name) {
        UIButton* button = getCppObjPtrInPikaObj<UIButton>(self);
        if (!button) return false;

        button->SetName(string(name));
        return true;
    }

    // SetColor method
    pika_bool _MatrixOS_UIButton_UIButton_SetColor(PikaObj *self, PikaObj* color) {
        UIButton* button = getCppObjPtrInPikaObj<UIButton>(self);
        if (!button) return false;

        Color* colorPtr = getCppObjPtrInPikaObj<Color>(color);
        if (!colorPtr) return false;

        button->SetColor(*colorPtr);
        return true;
    }

    // SetColorFunc method
    pika_bool _MatrixOS_UIButton_UIButton_SetColorFunc(PikaObj *self, Arg* colorFunc) {
        UIButton* button = getCppObjPtrInPikaObj<UIButton>(self);
        if (!button) return false;

        SaveCallbackObjToPikaObj(self, (char*)"colorFunc", colorFunc);

        button->SetColorFunc([self]() -> Color {
            Color retval = Color(0xFFFFFF);
            Arg* result = CallCallbackInPikaObj0(self, (char*)"colorFunc");

            if (result) {
                if (arg_getType(result) == ARG_TYPE_OBJECT) {
                    PikaObj* colorObj = arg_getObj(result);
                    Color* color = getCppObjPtrInPikaObj<Color>(colorObj);
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
        UIButton* button = getCppObjPtrInPikaObj<UIButton>(self);
        if (!button) return false;

        Dimension* dimPtr = getCppObjPtrInPikaObj<Dimension>(dimension);
        if (!dimPtr) return false;

        button->SetSize(*dimPtr);
        return true;
    }

    // OnPress method
    pika_bool _MatrixOS_UIButton_UIButton_OnPress(PikaObj *self, Arg* pressCallback) {
        UIButton* button = getCppObjPtrInPikaObj<UIButton>(self);
        if (!button) return false;

        SaveCallbackObjToPikaObj(self, (char*)"pressCallback", pressCallback);

        button->OnPress([self]() {
            Arg* result = CallCallbackInPikaObj0(self, (char*)"pressCallback");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }

    // OnHold method
    pika_bool _MatrixOS_UIButton_UIButton_OnHold(PikaObj *self, Arg* holdCallback) {
        UIButton* button = getCppObjPtrInPikaObj<UIButton>(self);
        if (!button) return false;

        SaveCallbackObjToPikaObj(self, (char*)"holdCallback", holdCallback);

        button->OnHold([self]() {
            Arg* result = CallCallbackInPikaObj0(self, (char*)"holdCallback");
            if (result) {
                arg_deinit(result);
            }
        });

        return true;
    }
}