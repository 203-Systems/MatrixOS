#include <functional>
#include "MatrixOS.h"
#include "UI/Component/UI4pxNumber.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../../PikaObjUtils.h"
#include "../../PikaCallbackUtils.h"

extern "C" {

    // UI4pxNumber constructor
    void _MatrixOS_UI4pxNumber_UI4pxNumber___init__(PikaObj *self) {
        createCppObjPtrInPikaObj<UI4pxNumber>(self);
    }

    // SetName method
    pika_bool _MatrixOS_UI4pxNumber_UI4pxNumber_SetName(PikaObj *self, char* name) {
        UI4pxNumber* number = getCppObjPtrInPikaObj<UI4pxNumber>(self);
        if (!number) return false;

        number->SetName(string(name));
        return true;
    }

    // SetColor method
    pika_bool _MatrixOS_UI4pxNumber_UI4pxNumber_SetColor(PikaObj *self, PikaObj* color) {
        UI4pxNumber* number = getCppObjPtrInPikaObj<UI4pxNumber>(self);
        if (!number) return false;

        Color* colorPtr = getCppObjPtrInPikaObj<Color>(color);
        if (!colorPtr) return false;

        number->SetColor(*colorPtr);
        return true;
    }

    // SetAlternativeColor method
    pika_bool _MatrixOS_UI4pxNumber_UI4pxNumber_SetAlternativeColor(PikaObj *self, PikaObj* alternativeColor) {
        UI4pxNumber* number = getCppObjPtrInPikaObj<UI4pxNumber>(self);
        if (!number) return false;

        Color* colorPtr = getCppObjPtrInPikaObj<Color>(alternativeColor);
        if (!colorPtr) return false;

        number->SetAlternativeColor(*colorPtr);
        return true;
    }

    // SetDigits method
    pika_bool _MatrixOS_UI4pxNumber_UI4pxNumber_SetDigits(PikaObj *self, int digits) {
        UI4pxNumber* number = getCppObjPtrInPikaObj<UI4pxNumber>(self);
        if (!number) return false;

        number->SetDigits((uint8_t)digits);
        return true;
    }

    // SetSpacing method
    pika_bool _MatrixOS_UI4pxNumber_UI4pxNumber_SetSpacing(PikaObj *self, int spacing) {
        UI4pxNumber* number = getCppObjPtrInPikaObj<UI4pxNumber>(self);
        if (!number) return false;

        number->SetSpacing((uint8_t)spacing);
        return true;
    }

    // SetValueFunc method
    pika_bool _MatrixOS_UI4pxNumber_UI4pxNumber_SetValueFunc(PikaObj *self, Arg* getValueFunc) {
        UI4pxNumber* number = getCppObjPtrInPikaObj<UI4pxNumber>(self);
        if (!number) return false;

        SaveCallbackObjToPikaObj(self, (char*)"getValueFunc", getValueFunc);

        number->SetValueFunc([self]() -> int32_t {
            int32_t retval = 0;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"getValueFunc");

            if (result) {
                if (arg_getType(result) == ARG_TYPE_INT) {
                    retval = arg_getInt(result);
                }
                arg_deinit(result);
            }

            return retval;
        });

        return true;
    }

    // SetColorFunc method
    pika_bool _MatrixOS_UI4pxNumber_UI4pxNumber_SetColorFunc(PikaObj *self, Arg* colorFunc) {
        UI4pxNumber* number = getCppObjPtrInPikaObj<UI4pxNumber>(self);
        if (!number) return false;

        SaveCallbackObjToPikaObj(self, (char*)"colorFunc", colorFunc);

        number->SetColorFunc([self](uint16_t digit) -> Color {
            Color retval = Color(0xFFFFFF);
            Arg* digitArg = arg_newInt(digit);
            Arg* result = CallCallbackInPikaObj1(self, (char*)"colorFunc", digitArg);

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
            arg_deinit(digitArg);

            return retval;
        });

        return true;
    }
}