#include <functional>
#include "MatrixOS.h"
#include "UI/Component/UISelector.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../../PikaObjUtils.h"
#include "../../PikaCallbackUtils.h"

extern "C" {

    // UISelector constructor
    void _MatrixOS_UISelector_UISelector___init__(PikaObj *self) {
        createCppObjPtrInPikaObj<UISelector>(self);
    }

    // SetValueFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetValueFunc(PikaObj *self, Arg* getValueFunc) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"getValueFunc", getValueFunc);

        selector->SetValueFunc([self]() -> uint16_t {
            uint16_t retval = 0;
            Arg* result = CallCallbackInPikaObj0(self, (char*)"getValueFunc");

            if (result) {
                if (arg_getType(result) == ARG_TYPE_INT) {
                    retval = (uint16_t)arg_getInt(result);
                }
                arg_deinit(result);
            }

            return retval;
        });

        return true;
    }

    // SetLitMode method
    pika_bool _MatrixOS_UISelector_UISelector_SetLitMode(PikaObj *self, int litMode) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        selector->SetLitMode((UISelectorLitMode)litMode);
        return true;
    }

    // SetDimension method
    pika_bool _MatrixOS_UISelector_UISelector_SetDimension(PikaObj *self, PikaObj* dimension) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        Dimension* dimPtr = getCppObjPtrInPikaObj<Dimension>(dimension);
        if (!dimPtr) return false;

        selector->SetDimension(*dimPtr);
        return true;
    }

    // SetName method
    pika_bool _MatrixOS_UISelector_UISelector_SetName(PikaObj *self, char* name) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        selector->SetName(string(name));
        return true;
    }

    // SetCount method
    pika_bool _MatrixOS_UISelector_UISelector_SetCount(PikaObj *self, int count) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        selector->SetCount((uint16_t)count);
        return true;
    }

    // SetDirection method
    pika_bool _MatrixOS_UISelector_UISelector_SetDirection(PikaObj *self, int direction) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        selector->SetDirection((UISelectorDirection)direction);
        return true;
    }

    // SetColor method
    pika_bool _MatrixOS_UISelector_UISelector_SetColor(PikaObj *self, PikaObj* color) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        Color* colorPtr = getCppObjPtrInPikaObj<Color>(color);
        if (!colorPtr) return false;

        selector->SetColor(*colorPtr);
        return true;
    }

    // OnChange method
    pika_bool _MatrixOS_UISelector_UISelector_OnChange(PikaObj *self, Arg* changeCallback) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"changeCallback", changeCallback);

        selector->OnChange([self](uint16_t value) {
            Arg* valueArg = arg_newInt(value);
            Arg* result = CallCallbackInPikaObj1(self, (char*)"changeCallback", valueArg);

            if (result) {
                arg_deinit(result);
            }
            arg_deinit(valueArg);
        });

        return true;
    }

    // SetColorFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetColorFunc(PikaObj *self, Arg* colorFunc) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"colorFunc", colorFunc);

        selector->SetColorFunc([self]() -> Color {
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

    // SetIndividualColorFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetIndividualColorFunc(PikaObj *self, Arg* colorFunc) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"colorFunc", colorFunc);

        selector->SetIndividualColorFunc([self](uint16_t index) -> Color {
            Color retval = Color(0xFFFFFF);
            Arg* indexArg = arg_newInt(index);
            Arg* result = CallCallbackInPikaObj1(self, (char*)"colorFunc", indexArg);

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
            arg_deinit(indexArg);

            return retval;
        });

        return true;
    }

    // SetNameFunc method
    pika_bool _MatrixOS_UISelector_UISelector_SetNameFunc(PikaObj *self, Arg* nameFunc) {
        UISelector* selector = getCppObjPtrInPikaObj<UISelector>(self);
        if (!selector) return false;

        SaveCallbackObjToPikaObj(self, (char*)"nameFunc", nameFunc);

        selector->SetIndividualNameFunc([self](uint16_t index) -> string {
            string retval = string("");
            Arg* indexArg = arg_newInt(index);
            Arg* result = CallCallbackInPikaObj1(self, (char*)"nameFunc", indexArg);

            if (result) {
                if (arg_getType(result) == ARG_TYPE_STRING) {
                    retval = string(arg_getStr(result));
                }
                arg_deinit(result);
            }
            arg_deinit(indexArg);

            return retval;
        });

        return true;
    }
}